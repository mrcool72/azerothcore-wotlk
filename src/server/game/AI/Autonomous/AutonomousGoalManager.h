#ifndef TRINITY_AUTONOMOUS_GOAL_MANAGER_H
#define TRINITY_AUTONOMOUS_GOAL_MANAGER_H

#include "Common.h"
#include <string>

namespace AutonomousAI
{
    struct Perception;

    class AutonomousGoalManager
    {
    public:
        void Update(uint32 diff, Perception const& perception, std::string const& routine);
        std::string const& GetGoal() const { return _goal; }
        uint8 GetProgress() const { return _progress; }

    private:
        uint32 _timer = 0;
        std::string _goal = "build a stable adventure routine";
        uint8 _progress = 0;
    };
}

#endif
