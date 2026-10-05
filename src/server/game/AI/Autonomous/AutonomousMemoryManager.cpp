#include "AutonomousAcoreCompat.h"
#include "AutonomousMemoryManager.h"
#include "DatabaseEnv.h"
#include "Player.h"

namespace AutonomousAI
{
    AutonomousMemoryManager::AutonomousMemoryManager(Player* player) :
        _player(player), _timer(0), _lastMap(0), _completedQuests(0), _lastCompletedQuest(0),
        _routine("idle"), _goal("build a stable adventure routine")
    {
        Load();
    }

    AutonomousMemoryManager::~AutonomousMemoryManager()
    {
        Save();
    }

    void AutonomousMemoryManager::Load()
    {
        if (!_player)
            return;
        if (QueryResult result = WorldDatabase.PQuery("SELECT last_map, completed_quests, last_completed_quest, routine, long_term_goal, visited_maps FROM autonomous_bot_memory WHERE guid = {}", _player->GetGUID().GetRawValue()))
        {
            Field* fields = result->Fetch();
            _lastMap = fields[0].GetUInt32();
            _completedQuests = fields[1].GetUInt32();
            _lastCompletedQuest = fields[2].GetUInt32();
            _routine = fields[3].GetString();
            _goal = fields[4].GetString();
            std::string visited = fields[5].GetString();
            size_t start = 0;
            while (start < visited.size())
            {
                size_t end = visited.find(',', start);
                uint32 map = static_cast<uint32>(std::stoul(visited.substr(start, end == std::string::npos ? std::string::npos : end - start)));
                _maps.insert(map);
                if (end == std::string::npos) break;
                start = end + 1;
            }
        }
    }

    void AutonomousMemoryManager::Save()
    {
        if (!_player)
            return;
        std::string visited;
        for (uint32 map : _maps)
        {
            if (!visited.empty()) visited += ',';
            visited += std::to_string(map);
        }
        WorldDatabase.PExecute("REPLACE INTO autonomous_bot_memory (guid, last_map, completed_quests, last_completed_quest, routine, long_term_goal, visited_maps) VALUES ({}, {}, {}, {}, '{}', '{}', '{}')",
            _player->GetGUID().GetRawValue(), _lastMap, _completedQuests, _lastCompletedQuest, _routine, _goal, visited);
    }

    void AutonomousMemoryManager::Update(uint32 diff, uint32 mapId, uint32 activeQuest, uint32 lastCompletedQuest,
        uint32 completedQuestCount, std::string const& routine, std::string const& goal)
    {
        if (!_player || !_player->IsInWorld()) return;
        _maps.insert(mapId);
        _lastMap = mapId;
        if (lastCompletedQuest) _lastCompletedQuest = lastCompletedQuest;
        _completedQuests = completedQuestCount;
        _routine = routine;
        _goal = goal;
        (void)activeQuest;
        if (_timer > diff) { _timer -= diff; return; }
        _timer = 30000;
        Save();
    }
}
