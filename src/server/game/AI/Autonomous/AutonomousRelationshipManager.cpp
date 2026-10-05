#include "AutonomousAcoreCompat.h"
#include "AutonomousRelationshipManager.h"
#include "DatabaseEnv.h"

#include <algorithm>

namespace AutonomousAI
{
    AutonomousRelationshipManager sAutonomousRelationshipMgr;

    uint64 AutonomousRelationshipManager::MakeKey(uint64 botGuid, uint64 otherGuid)
    {
        // Two 32-bit halves are sufficient for TrinityCore's practical character GUID range.
        return (botGuid << 32) ^ (otherGuid & 0xFFFFFFFFULL);
    }

    RelationshipMemory AutonomousRelationshipManager::Load(uint64 botGuid, uint64 otherGuid)
    {
        RelationshipMemory memory;
        if (QueryResult result = WorldDatabase.PQuery(
                "SELECT score, interactions, preferred FROM autonomous_bot_relationships WHERE bot_guid = {} AND other_guid = {}",
                botGuid, otherGuid))
        {
            Field* fields = result->Fetch();
            memory.score = fields[0].GetInt32();
            memory.interactions = fields[1].GetUInt32();
            memory.preferred = fields[2].GetUInt8() != 0;
        }
        return memory;
    }

    void AutonomousRelationshipManager::Save(uint64 botGuid, uint64 otherGuid, RelationshipMemory const& memory)
    {
        WorldDatabase.PExecute(
            "REPLACE INTO autonomous_bot_relationships (bot_guid, other_guid, score, interactions, preferred) VALUES ({}, {}, {}, {}, {})",
            botGuid, otherGuid, memory.score, memory.interactions, memory.preferred ? 1 : 0);
    }

    RelationshipMemory AutonomousRelationshipManager::Get(uint64 botGuid, uint64 otherGuid)
    {
        uint64 key = MakeKey(botGuid, otherGuid);
        auto itr = _cache.find(key);
        if (itr != _cache.end())
            return itr->second;

        RelationshipMemory memory = Load(botGuid, otherGuid);
        _cache.emplace(key, memory);
        return memory;
    }

    int32 AutonomousRelationshipManager::GetScore(uint64 botGuid, uint64 otherGuid)
    {
        return Get(botGuid, otherGuid).score;
    }

    void AutonomousRelationshipManager::RecordInteraction(uint64 botGuid, uint64 otherGuid, int32 delta)
    {
        if (!botGuid || !otherGuid || botGuid == otherGuid)
            return;

        uint64 key = MakeKey(botGuid, otherGuid);
        RelationshipMemory& memory = _cache[key];
        if (!memory.interactions && memory.score == 0 && !memory.preferred)
            memory = Load(botGuid, otherGuid);

        memory.score = std::max(-100, std::min(100, memory.score + delta));
        ++memory.interactions;
        Save(botGuid, otherGuid, memory);
        _summaries.erase(botGuid);
    }

    void AutonomousRelationshipManager::SetPreferred(uint64 botGuid, uint64 otherGuid, bool preferred)
    {
        if (!botGuid || !otherGuid || botGuid == otherGuid)
            return;

        uint64 key = MakeKey(botGuid, otherGuid);
        RelationshipMemory& memory = _cache[key];
        if (!memory.interactions && memory.score == 0 && !memory.preferred)
            memory = Load(botGuid, otherGuid);
        memory.preferred = preferred;
        Save(botGuid, otherGuid, memory);
        _summaries.erase(botGuid);
    }

    uint64 AutonomousRelationshipManager::GetPreferredCompanion(uint64 botGuid)
    {
        Summary& summary = _summaries[botGuid];
        if (!summary.loaded)
        {
            if (QueryResult result = WorldDatabase.PQuery(
                    "SELECT other_guid, score FROM autonomous_bot_relationships WHERE bot_guid = {} AND (preferred = 1 OR score >= 0) ORDER BY preferred DESC, score DESC, interactions DESC LIMIT 1",
                    botGuid))
            {
                Field* fields = result->Fetch();
                summary.preferredGuid = fields[0].GetUInt64();
                summary.score = fields[1].GetInt32();
            }
            if (QueryResult result = WorldDatabase.PQuery(
                    "SELECT COALESCE(SUM(score), 0), COALESCE(SUM(interactions), 0) FROM autonomous_bot_relationships WHERE bot_guid = {}",
                    botGuid))
            {
                Field* fields = result->Fetch();
                summary.score = fields[0].GetInt32();
                summary.interactions = fields[1].GetUInt32();
            }
            summary.loaded = true;
        }
        return summary.preferredGuid;
    }

    int32 AutonomousRelationshipManager::GetSocialScore(uint64 botGuid)
    {
        GetPreferredCompanion(botGuid);
        return _summaries[botGuid].score;
    }

    uint32 AutonomousRelationshipManager::GetInteractionCount(uint64 botGuid)
    {
        GetPreferredCompanion(botGuid);
        return _summaries[botGuid].interactions;
    }
}
