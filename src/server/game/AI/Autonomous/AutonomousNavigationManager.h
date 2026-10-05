#ifndef TRINITY_AUTONOMOUS_NAVIGATION_MANAGER_H
#define TRINITY_AUTONOMOUS_NAVIGATION_MANAGER_H

#include "Common.h"
#include "AutonomousBotProtocol.h"
#include <string>

namespace AutonomousAI
{
    class AutonomousNavigationManager
    {
    public:
        void Update(uint32 diff, Perception const& perception);
        bool HasPlan() const { return _hasPlan; }
        uint32 GetStep() const { return _step; }
        std::string const& GetStage() const { return _stage; }
        std::string const& GetReason() const { return _reason; }
        Position const& GetDestination() const { return _destination; }

    private:
        uint32 _timer = 0;
        uint32 _step = 0;
        bool _hasPlan = false;
        std::string _stage = "idle";
        std::string _reason = "none";
        Position _destination;
    };
}

#endif
