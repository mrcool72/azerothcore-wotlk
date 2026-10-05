#ifndef TRINITY_AUTONOMOUS_SERVICE_MANAGER_H
#define TRINITY_AUTONOMOUS_SERVICE_MANAGER_H

#include "Common.h"
#include "AutonomousBotProtocol.h"

class Player;

namespace AutonomousAI
{
    class AutonomousServiceManager
    {
    public:
        explicit AutonomousServiceManager(Player* player);
        void Update(uint32 diff, Perception const& perception, bool externalActive);
        void RequestService(std::string service, uint64 guid = 0) { _requestedService = service; _requestedGuid = guid; }

        uint32 GetSoldJunk() const { return _soldJunk; }
        uint32 GetRepaired() const { return _repaired; }
        bool GetBankOpened() const { return _bankOpened; }
        std::string const& GetLastService() const { return _lastService; }

    private:
        Player* _player;
        uint32 _timer;
        uint32 _soldJunk;
        uint32 _repaired;
        bool _bankOpened;
        std::string _lastService;
        std::string _requestedService = "";
        uint64 _requestedGuid = 0;
    };
}

#endif
