#ifndef TRINITY_AUTONOMOUS_LIFE_MANAGER_H
#define TRINITY_AUTONOMOUS_LIFE_MANAGER_H

#include "Common.h"
#include <string>

namespace AutonomousAI
{
    struct Perception;

    class AutonomousLifeManager
    {
    public:
        void Update(uint32 diff, Perception const& perception, std::string const& routine, std::string const& goal);
        std::string const& GetState() const { return _state; }
        std::string const& GetSchedule() const { return _schedule; }
        uint32 GetStateAge() const { return _stateAge; }
        uint32 GetCycle() const { return _cycle; }

    private:
        uint32 _timer = 0;
        uint32 _cycleTimer = 0;
        uint32 _stateAge = 0;
        uint32 _cycle = 0;
        std::string _state = "idle";
        std::string _schedule = "adventure";
    };
}

#endif
