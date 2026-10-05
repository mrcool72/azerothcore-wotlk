#ifndef TRINITY_AUTONOMOUS_ROUTINE_MANAGER_H
#define TRINITY_AUTONOMOUS_ROUTINE_MANAGER_H

#include "Common.h"
#include <string>

namespace AutonomousAI
{
    struct Perception;

    class AutonomousRoutineManager
    {
    public:
        void Update(uint32 diff, Perception const& perception);
        std::string const& GetRoutine() const { return _routine; }
        uint32 GetRoutineAge() const { return _routineAge; }

    private:
        uint32 _timer = 0;
        uint32 _routineAge = 0;
        std::string _routine = "idle";
    };
}

#endif
