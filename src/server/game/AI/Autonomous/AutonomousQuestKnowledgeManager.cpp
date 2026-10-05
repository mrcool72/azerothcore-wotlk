#include "AutonomousAcoreCompat.h"
#include "AutonomousQuestKnowledgeManager.h"
#include "DatabaseEnv.h"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <limits>

namespace AutonomousAI
{
AutonomousQuestKnowledgeManager sAutonomousQuestKnowledgeMgr;

uint32 AutonomousQuestKnowledgeManager::Cell(float coordinate)
{
    // 25-yard cells provide several distinct hunting areas without turning
    // every observation into a unique database row.
    return static_cast<uint32>(std::floor(coordinate / 25.0f) + 100000.0f);
}

uint64 AutonomousQuestKnowledgeManager::MakeKey(uint32 questId, uint32 entry, uint32 mapId, uint32 cellX, uint32 cellY)
{
    uint64 key = (static_cast<uint64>(questId) << 32) ^ static_cast<uint64>(entry);
    key ^= static_cast<uint64>(mapId) * 0x9E3779B185EBCA87ULL;
    key ^= static_cast<uint64>(cellX) * 0xC2B2AE3D27D4EB4FULL;
    key ^= static_cast<uint64>(cellY) * 0x165667B19E3779F9ULL;
    return key;
}

void AutonomousQuestKnowledgeManager::LoadQuest(uint32 questId, uint32 entry, uint32 mapId)
{
    LoadLocations(questId, entry);
    (void)mapId;
}

void AutonomousQuestKnowledgeManager::LoadLocations(uint32 questId, uint32 entry)
{
    if (QueryResult result = WorldDatabase.Query(
            "SELECT map_id, cell_x, cell_y, x, y, z, o, observations, last_seen FROM autonomous_bot_quest_locations WHERE quest_id = {} AND entry = {}",
            questId, entry))
    {
        do
        {
            Field* fields = result->Fetch();
            QuestWorldKnowledge knowledge;
            knowledge.questId = questId;
            knowledge.entry = entry;
            knowledge.mapId = fields[0].GetUInt32();
            knowledge.cellX = fields[1].GetUInt32();
            knowledge.cellY = fields[2].GetUInt32();
            knowledge.position.mapId = knowledge.mapId;
            knowledge.position.x = fields[3].GetFloat();
            knowledge.position.y = fields[4].GetFloat();
            knowledge.position.z = fields[5].GetFloat();
            knowledge.position.orientation = fields[6].GetFloat();
            knowledge.observations = fields[7].GetUInt32();
            knowledge.lastSeen = fields[8].GetUInt64();
            knowledge.confidence = std::min<uint32>(100, knowledge.observations * 15);
            knowledge.density = knowledge.observations;
            _cache[MakeKey(questId, entry, knowledge.mapId, knowledge.cellX, knowledge.cellY)] = knowledge;
        } while (result->NextRow());
    }

    // Read legacy Phase 61 knowledge as a fallback so an existing database is
    // not discarded when the multi-location table is introduced.
    if (QueryResult legacy = WorldDatabase.PQuery(
            "SELECT map_id, x, y, z, o, observations, last_seen FROM autonomous_bot_quest_knowledge WHERE quest_id = {} AND entry = {}",
            questId, entry))
    {
        do
        {
            Field* fields = legacy->Fetch();
            QuestWorldKnowledge knowledge;
            knowledge.questId = questId;
            knowledge.entry = entry;
            knowledge.mapId = fields[0].GetUInt32();
            knowledge.position.mapId = knowledge.mapId;
            knowledge.position.x = fields[1].GetFloat();
            knowledge.position.y = fields[2].GetFloat();
            knowledge.position.z = fields[3].GetFloat();
            knowledge.position.orientation = fields[4].GetFloat();
            knowledge.observations = fields[5].GetUInt32();
            knowledge.lastSeen = fields[6].GetUInt64();
            knowledge.cellX = Cell(knowledge.position.x);
            knowledge.cellY = Cell(knowledge.position.y);
            knowledge.confidence = std::min<uint32>(100, knowledge.observations * 15);
            knowledge.density = knowledge.observations;
            uint64 key = MakeKey(questId, entry, knowledge.mapId, knowledge.cellX, knowledge.cellY);
            if (_cache.find(key) == _cache.end())
                _cache.emplace(key, knowledge);
        } while (legacy->NextRow());
    }
}

void AutonomousQuestKnowledgeManager::Save(QuestWorldKnowledge const& knowledge)
{
    WorldDatabase.Execute(
        "REPLACE INTO autonomous_bot_quest_locations (quest_id, entry, map_id, cell_x, cell_y, x, y, z, o, observations, last_seen) VALUES ({}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {})",
        knowledge.questId, knowledge.entry, knowledge.mapId, knowledge.cellX, knowledge.cellY,
        knowledge.position.x, knowledge.position.y, knowledge.position.z,
        knowledge.position.orientation, knowledge.observations, knowledge.lastSeen);
}

void AutonomousQuestKnowledgeManager::Update(uint32 diff, Perception const& perception)
{
    if (_timer > diff)
    {
        _timer -= diff;
        return;
    }
    _timer = 2000;

    for (QuestObjectiveInfo const& quest : perception.activeQuests)
    {
        for (size_t i = 0; i < quest.requiredCreatureOrGameObject.size(); ++i)
        {
            uint32 entry = quest.requiredCreatureOrGameObject[i];
            if (entry >= 0x80000000u)
                entry = static_cast<uint32>(-static_cast<int32>(entry));
            uint32 progress = i < quest.creatureOrGameObjectProgress.size() ? quest.creatureOrGameObjectProgress[i] : 0;
            uint32 needed = i < quest.requiredCreatureOrGameObjectCount.size() ? quest.requiredCreatureOrGameObjectCount[i] : 0;
            if (!entry || (needed && progress >= needed))
                continue;

            auto observe = [&](WorldObjectInfo const& object)
            {
                if (!object.entry || object.entry != entry)
                    return;
                if (object.position.mapId != perception.mapId)
                    return;

                uint32 cellX = Cell(object.position.x);
                uint32 cellY = Cell(object.position.y);
                uint64 key = MakeKey(quest.questId, entry, object.position.mapId, cellX, cellY);
                auto itr = _cache.find(key);
                if (itr == _cache.end())
                {
                    LoadQuest(quest.questId, entry, object.position.mapId);
                    itr = _cache.find(key);
                }
                if (itr == _cache.end())
                    itr = _cache.emplace(key, QuestWorldKnowledge()).first;

                QuestWorldKnowledge knowledge = itr->second;
                knowledge.questId = quest.questId;
                knowledge.entry = entry;
                knowledge.mapId = object.position.mapId;
                knowledge.position = object.position;
                knowledge.cellX = cellX;
                knowledge.cellY = cellY;
                ++knowledge.observations;
                knowledge.density = knowledge.observations;
                knowledge.lastSeen = static_cast<uint64>(std::time(nullptr));
                knowledge.confidence = std::min<uint32>(100, knowledge.observations * 15);
                itr->second = knowledge;
                Save(knowledge);
            };

            for (WorldObjectInfo const& object : perception.nearbyCreatures)
            {
                if (object.alive)
                    observe(object);
            }
            for (WorldObjectInfo const& object : perception.nearbyGameObjects)
                observe(object);
        }
    }
}

bool AutonomousQuestKnowledgeManager::FindBest(uint32 questId, uint32 entry, uint32 preferredMapId,
    Position const* origin, QuestWorldKnowledge& result) const
{
    bool found = false;
    double bestScore = -std::numeric_limits<double>::infinity();
    uint64 now = static_cast<uint64>(std::time(nullptr));

    for (auto const& pair : _cache)
    {
        QuestWorldKnowledge const& knowledge = pair.second;
        if (knowledge.questId != questId || knowledge.entry != entry)
            continue;

        double score = static_cast<double>(knowledge.confidence) * 2.0;
        if (knowledge.mapId == preferredMapId)
            score += 35.0;

        uint64 age = now > knowledge.lastSeen ? now - knowledge.lastSeen : 0;
        score += age < 300 ? 25.0 : age < 1800 ? 10.0 : -10.0;
        score += std::min<double>(20.0, static_cast<double>(knowledge.density) * 2.0);

        if (origin && knowledge.mapId == origin->mapId)
        {
            float dx = knowledge.position.x - origin->x;
            float dy = knowledge.position.y - origin->y;
            float dz = knowledge.position.z - origin->z;
            double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
            score += std::max(-30.0, 30.0 - distance / 100.0);
        }

        if (!found || score > bestScore)
        {
            bestScore = score;
            result = knowledge;
            found = true;
        }
    }
    return found;
}

bool AutonomousQuestKnowledgeManager::Find(uint32 questId, uint32 entry, QuestWorldKnowledge& result) const
{
    return FindBest(questId, entry, 0, nullptr, result);
}

uint32 AutonomousQuestKnowledgeManager::GetLocationCount(uint32 questId, uint32 entry) const
{
    uint32 count = 0;
    for (auto const& pair : _cache)
        if (pair.second.questId == questId && pair.second.entry == entry)
            ++count;
    return count;
}
}
