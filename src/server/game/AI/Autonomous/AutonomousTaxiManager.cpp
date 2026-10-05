#include "AutonomousAcoreCompat.h"
#include "AutonomousTaxiManager.h"

#include "Creature.h"
#include "DBCStores.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"

namespace AutonomousAI
{
    AutonomousTaxiManager::AutonomousTaxiManager(Player* player) :
        _player(player), _timer(0), _sourceNode(0), _destinationNode(0), _destinationMap(0),
        _requestedSource(0), _requestedDestination(0), _flightMasterGuid(), _flightMasterDistance(0.0f),
        _routeActive(false), _state("idle")
    {
    }

    uint32 AutonomousTaxiManager::FindNearestTaxiNode() const
    {
        if (!_player)
            return 0;
        return sObjectMgr->GetNearestTaxiNode(_player->GetPositionX(), _player->GetPositionY(),
            _player->GetPositionZ(), _player->GetMapId(), _player->GetTeamId());
    }

    bool AutonomousTaxiManager::StartRequestedRoute()
    {
        if (!_player || !_requestedSource || !_requestedDestination || !_player->IsAlive() || _player->IsInCombat())
            return false;

        Creature* flightMaster = ObjectAccessor::GetCreature(*_player, _flightMasterGuid);
        if (!flightMaster || !flightMaster->HasNpcFlag(UNIT_NPC_FLAG_FLIGHTMASTER) ||
            !_player->IsWithinDistInMap(flightMaster, 8.0f))
            return false;

        if (_requestedSource != FindNearestTaxiNode())
            return false;

        std::vector<uint32> nodes{ _requestedSource, _requestedDestination };
        if (!_player->ActivateTaxiPathTo(nodes, flightMaster))
            return false;

        _sourceNode = _requestedSource;
        _destinationNode = _requestedDestination;
        if (TaxiNodesEntry const* node = sTaxiNodesStore.LookupEntry(_destinationNode))
            _destinationMap = node->map_id;
        _routeActive = true;
        _state = "flying";
        _requestedSource = 0;
        _requestedDestination = 0;
        return true;
    }

    bool AutonomousTaxiManager::ChooseLocalExplorationRoute(Perception const& perception)
    {
        if (!_player || perception.inDungeon || perception.groupInCombat || _player->IsInCombat())
            return false;
        if (perception.worldRoutine != "exploring" && perception.lifeSchedule != "exploration")
            return false;

        uint32 source = FindNearestTaxiNode();
        if (!source)
            return false;

        TaxiNodesEntry const* sourceNode = sTaxiNodesStore.LookupEntry(source);
        if (!sourceNode)
            return false;

        // Pick the first known direct taxi destination on another map. This is a
        // deterministic fallback for local autonomous exploration; external AI can
        // override it with an explicit route.
        for (uint32 id = 1; id < sTaxiNodesStore.GetNumRows(); ++id)
        {
            TaxiNodesEntry const* node = sTaxiNodesStore.LookupEntry(id);
            if (!node || node->map_id == sourceNode->map_id || !node->ID)
                continue;
            if (!_player->m_taxi.IsTaximaskNodeKnown(id))
                continue;

            uint32 path = 0;
            uint32 cost = 0;
            sObjectMgr->GetTaxiPath(source, id, path, cost);
            if (!path)
                continue;

            _requestedSource = source;
            _requestedDestination = id;
            _state = "route_selected";
            return true;
        }

        return false;
    }

bool AutonomousTaxiManager::RequestCampaignTravel(uint32 targetMap)
{
    if (!_player || !_player->IsAlive() || _player->IsInCombat() || _player->IsInFlight())
        return false;

    uint32 source = FindNearestTaxiNode();
    if (!source)
        return false;

    TaxiNodesEntry const* sourceNode = sTaxiNodesStore.LookupEntry(source);
    if (!sourceNode)
        return false;

    // We do not invent quest coordinates. When the campaign target is outside
    // current perception, use a known, reachable taxi node on another map as a
    // deterministic search hop. Once there, normal quest perception is rebuilt.
    for (uint32 id = 1; id < sTaxiNodesStore.GetNumRows(); ++id)
    {
        TaxiNodesEntry const* node = sTaxiNodesStore.LookupEntry(id);
        if (!node || !node->ID || node->map_id == sourceNode->map_id)
            continue;
        if (targetMap && node->map_id != targetMap)
            continue;
        if (!_player->m_taxi.IsTaximaskNodeKnown(id))
            continue;

        uint32 path = 0;
        uint32 cost = 0;
        sObjectMgr->GetTaxiPath(source, id, path, cost);
        if (!path)
            continue;

        _requestedSource = source;
        _requestedDestination = id;
        _state = "campaign_route_selected";
        return true;
    }

    return false;
}

    void AutonomousTaxiManager::Update(uint32 diff, Perception const& perception, bool externalActive)
    {
        if (!_player || !_player->IsInWorld() || !_player->IsAlive())
            return;
        if (_player->IsInFlight())
        {
            _routeActive = true;
            _state = "flying";
            return;
        }

        if (_timer > diff)
        {
            _timer -= diff;
            return;
        }
        _timer = 2000;

        if (_routeActive)
        {
            _routeActive = false;
            _state = "arrived";
        }

        if (!_requestedSource && !externalActive)
            ChooseLocalExplorationRoute(perception);

        if (!_requestedSource || !_requestedDestination)
            return;

        // Locate a nearby flight master through the already serialized perception.
        _flightMasterGuid = ObjectGuid::Empty;
        _flightMasterDistance = 9999.0f;
        for (WorldObjectInfo const& object : perception.nearbyCreatures)
        {
            if (!object.guid || object.distance >= _flightMasterDistance)
                continue;
            Creature* creature = ObjectAccessor::GetCreature(*_player, AutonomousMakeGuid(object.guid));
            if (!creature || !creature->HasNpcFlag(UNIT_NPC_FLAG_FLIGHTMASTER))
                continue;
            _flightMasterGuid = creature->GetGUID();
            _flightMasterDistance = object.distance;
        }

        if (_flightMasterGuid.IsEmpty())
        {
            _state = "find_flightmaster";
            return;
        }

        if (_flightMasterDistance > 6.0f)
        {
            if (Creature* flightMaster = ObjectAccessor::GetCreature(*_player, _flightMasterGuid))
                _player->GetMotionMaster()->MovePoint(0, flightMaster->GetPositionX(), flightMaster->GetPositionY(), flightMaster->GetPositionZ(), FORCED_MOVEMENT_NONE);
            _state = "travel_to_flightmaster";
            return;
        }

        StartRequestedRoute();
    }
}
