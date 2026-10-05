#ifndef TRINITY_AUTONOMOUS_TRAVEL_MANAGER_H
#define TRINITY_AUTONOMOUS_TRAVEL_MANAGER_H

#include "AutonomousBotProtocol.h"
#include "Common.h"
#include <string>

class Player;

namespace AutonomousAI
{
    class AutonomousTravelManager
    {
    public:
        explicit AutonomousTravelManager(Player* player);
        void Update(uint32 diff, Perception const& perception, bool externalActive);
        bool IsTraveling() const { return _traveling; }
        Position const& GetDestination() const { return _destination; }
        std::string const& GetReason() const { return _reason; }

    private:
        void MoveTo(Position const& position);
        Player* _player;
        uint32 _timer;
        bool _traveling;
        Position _destination;
        std::string _reason;
    };
}
#endif
