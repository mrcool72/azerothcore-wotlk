#ifndef TRINITY_AUTONOMOUS_MEMORY_MANAGER_H
#define TRINITY_AUTONOMOUS_MEMORY_MANAGER_H

#include "Common.h"
#include <string>
#include <unordered_set>

class Player;

namespace AutonomousAI
{
    class AutonomousMemoryManager
    {
    public:
        explicit AutonomousMemoryManager(Player* player);
        ~AutonomousMemoryManager();
        void Update(uint32 diff, uint32 mapId, uint32 activeQuest, uint32 lastCompletedQuest,
            uint32 completedQuestCount, std::string const& routine, std::string const& goal);
        uint32 GetVisitedMaps() const { return static_cast<uint32>(_maps.size()); }
        uint32 GetCompletedQuests() const { return _completedQuests; }
        uint32 GetLastCompletedQuest() const { return _lastCompletedQuest; }
        uint32 GetLastMap() const { return _lastMap; }

    private:
        void Load();
        void Save();
        Player* _player;
        uint32 _timer;
        uint32 _lastMap;
        uint32 _completedQuests;
        uint32 _lastCompletedQuest;
        std::string _routine;
        std::string _goal;
        std::unordered_set<uint32> _maps;
    };
}
#endif
