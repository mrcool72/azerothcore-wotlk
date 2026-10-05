#include "AutonomousRoutineManager.h"
#include "AutonomousBotProtocol.h"

namespace AutonomousAI
{
    void AutonomousRoutineManager::Update(uint32 diff, Perception const& perception)
    {
        _routineAge += diff;
        if (_timer > diff)
        {
            _timer -= diff;
            return;
        }
        _timer = 5000;

        std::string next = "exploring";
        if (!perception.health || (perception.maxHealth && perception.health * 100u / perception.maxHealth < 45u))
            next = "recovery";
        else if (perception.inDungeon)
            next = "dungeon";
        else if (perception.needsTown)
            next = "town";
        else if (perception.groupMembers.size() > 1)
            next = "social";
        else if (!perception.activeQuests.empty() || perception.worldRecommendedQuest)
            next = "questing";

        // Avoid thrashing between routines when a transient perception changes for one tick.
        if (next != _routine && _routineAge >= 10000)
        {
            _routine = next;
            _routineAge = 0;
        }
    }
}
