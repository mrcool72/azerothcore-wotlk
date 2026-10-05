#ifndef TRINITY_AUTONOMOUS_TAXI_MANAGER_H
#define TRINITY_AUTONOMOUS_TAXI_MANAGER_H

#include "Common.h"
#include "AutonomousBotProtocol.h"

class Player;
class Creature;

namespace AutonomousAI
{
    class AutonomousTaxiManager
    {
    public:
        explicit AutonomousTaxiManager(Player* player);
        void Update(uint32 diff, Perception const& perception, bool externalActive);

        bool HasTaxiRoute() const { return _routeActive; }
        uint32 GetSourceNode() const { return _sourceNode; }
        uint32 GetDestinationNode() const { return _destinationNode; }
        uint32 GetDestinationMap() const { return _destinationMap; }
        float GetFlightMasterDistance() const { return _flightMasterDistance; }
        std::string const& GetState() const { return _state; }
        void RequestRoute(uint32 sourceNode, uint32 destinationNode) { _requestedSource = sourceNode; _requestedDestination = destinationNode; }
        bool RequestCampaignTravel(uint32 targetMap = 0);
        void ClearRoute() { _requestedSource = 0; _requestedDestination = 0; _routeActive = false; }

    private:
        bool StartRequestedRoute();
        bool ChooseLocalExplorationRoute(Perception const& perception);
        uint32 FindNearestTaxiNode() const;

        Player* _player;
        uint32 _timer;
        uint32 _sourceNode;
        uint32 _destinationNode;
        uint32 _destinationMap;
        uint32 _requestedSource;
        uint32 _requestedDestination;
        ObjectGuid _flightMasterGuid;
        float _flightMasterDistance;
        bool _routeActive;
        std::string _state;
    };
}

#endif
