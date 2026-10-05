#ifndef TRINITY_AUTONOMOUS_WORLD_MANAGER_H
#define TRINITY_AUTONOMOUS_WORLD_MANAGER_H

#include "Common.h"
#include "AutonomousRoutineManager.h"
#include "AutonomousGoalManager.h"
#include <string>
#include <unordered_map>
#include <memory>

class Player;

namespace AutonomousAI
{
    struct Perception;
    class AutonomousWorldManager
    {
    public:
        explicit AutonomousWorldManager(Player* player);
        void Update(uint32 diff, Perception const& perception);
        std::string GetRoutine() const { return _routine; }
        std::string GetLongTermGoal() const { return _longTermGoal; }
        uint32 GetCompletedQuestCount() const { return _completedQuests; }
        uint32 GetLastCompletedQuest() const { return _lastCompletedQuest; }
        uint32 GetVisitedMapCount() const { return _visitedMaps; }
        uint32 GetRecommendedQuest() const { return _recommendedQuest; }
        uint8 GetGoalProgress() const { return _goalProgress; }

    private:
        void RememberMap(uint32 mapId);
        Player* _player;
        uint32 _timer;
        uint32 _previousActiveQuest;
        uint32 _completedQuests;
        uint32 _lastCompletedQuest;
        uint32 _visitedMaps;
        uint32 _lastMapId;
        uint32 _recommendedQuest;
        std::string _routine;
        std::string _longTermGoal;
        std::unordered_map<uint32, bool> _maps;
        std::unique_ptr<AutonomousRoutineManager> _routineManager;
        std::unique_ptr<AutonomousGoalManager> _goalManager;
        uint8 _goalProgress = 0;
    };
}
#endif
