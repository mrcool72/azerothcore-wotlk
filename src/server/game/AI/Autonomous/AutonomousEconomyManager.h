#ifndef TRINITY_AUTONOMOUS_ECONOMY_MANAGER_H
#define TRINITY_AUTONOMOUS_ECONOMY_MANAGER_H

#include "Common.h"
#include <string>

namespace AutonomousAI
{
    struct Perception;

    class AutonomousEconomyManager
    {
    public:
        void Update(uint32 diff, Perception const& perception);
        bool NeedsEconomy() const { return _needsEconomy; }
        uint8 GetPressure() const { return _pressure; }
        std::string const& GetReason() const { return _reason; }

    private:
        uint32 _timer = 0;
        bool _needsEconomy = false;
        uint8 _pressure = 0;
        std::string _reason = "none";
    };
}

#endif
