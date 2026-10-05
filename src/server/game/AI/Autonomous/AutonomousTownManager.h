#ifndef TRINITY_AUTONOMOUS_TOWN_MANAGER_H
#define TRINITY_AUTONOMOUS_TOWN_MANAGER_H

#include "AutonomousBotProtocol.h"
#include "Common.h"
#include <string>

class Player;

namespace AutonomousAI
{
    class AutonomousTownManager
    {
    public:
        explicit AutonomousTownManager(Player* player);
        void Update(uint32 diff, Perception const& perception, bool externalActive);
        bool NeedsTown() const { return _needsTown; }
        uint64 GetServiceGuid() const { return _serviceGuid; }
        std::string const& GetServiceType() const { return _serviceType; }

    private:
        Player* _player;
        uint32 _timer;
        bool _needsTown;
        uint64 _serviceGuid;
        std::string _serviceType;
    };
}
#endif
