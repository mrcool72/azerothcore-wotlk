#include "AutonomousLifeManager.h"
#include "AutonomousBotProtocol.h"

namespace AutonomousAI
{
    void AutonomousLifeManager::Update(uint32 diff, Perception const& perception, std::string const& routine, std::string const& goal)
    {
        _stateAge += diff;
        _cycleTimer += diff;
        if (_timer > diff)
        {
            _timer -= diff;
            return;
        }
        _timer = 5000;

        // A 24-hour real-time cycle is represented as four broad life periods.
        // This is intentionally server-side and deterministic; it is not a claim
        // about the client's in-game clock.
        constexpr uint32 CYCLE_MS = 24u * 60u * 60u * 1000u;
        constexpr uint32 PERIOD_MS = CYCLE_MS / 4u;
        if (_cycleTimer >= PERIOD_MS)
        {
            _cycleTimer %= PERIOD_MS;
            _cycle = (_cycle + 1) % 4;
        }

        if (!perception.health || (perception.maxHealth && perception.health * 100u / perception.maxHealth < 30u))
            _schedule = "rest";
        else if (perception.inDungeon)
            _schedule = "adventure";
        else if (routine == "social")
            _schedule = "social";
        else if (routine == "town")
            _schedule = "town";
        else if (routine == "exploring")
            _schedule = "exploration";
        else
            _schedule = "adventure";

        std::string next = _schedule;
        if (goal == "finish the dungeon" || routine == "dungeon")
            next = "dungeon";
        else if (routine == "recovery")
            next = "rest";
        else if (routine == "town")
            next = "town";
        else if (routine == "social")
            next = "social";
        else if (routine == "exploring")
            next = "explore";
        else if (!perception.activeQuests.empty())
            next = "quest";

        if (next != _state && _stateAge >= 10000)
        {
            _state = next;
            _stateAge = 0;
        }

    }
}
