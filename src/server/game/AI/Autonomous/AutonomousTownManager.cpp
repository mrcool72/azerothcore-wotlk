#include "AutonomousAcoreCompat.h"
#include "AutonomousTownManager.h"
#include "Creature.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "Player.h"

namespace AutonomousAI
{
    AutonomousTownManager::AutonomousTownManager(Player* player) :
        _player(player), _timer(0), _needsTown(false), _serviceGuid(0), _serviceType("none")
    {
    }

    void AutonomousTownManager::Update(uint32 diff, Perception const& perception, bool externalActive)
    {
        if (!_player || !_player->IsInWorld() || !_player->IsAlive())
            return;
        if (_timer > diff) { _timer -= diff; return; }
        _timer = 3000;

        _needsTown = false;
        _serviceGuid = 0;
        _serviceType = "none";

        // Keep the decision conservative: only seek town services when inventory
        // space is nearly exhausted. The external AI may override the destination.
        if (externalActive || _player->GetFreeInventorySpace() > 2 || perception.inDungeon)
            return;

        for (WorldObjectInfo const& object : perception.nearbyCreatures)
        {
            Creature* creature = ObjectAccessor::GetCreature(*_player, AutonomousMakeGuid(object.guid));
            if (!creature || object.distance > 60.0f)
                continue;
            if (creature->IsVendor())
            {
                _needsTown = true;
                _serviceGuid = object.guid;
                _serviceType = "vendor";
                _player->GetMotionMaster()->MovePoint(0, object.position.x, object.position.y, object.position.z, FORCED_MOVEMENT_NONE, 0.0f, object.position.orientation);
                return;
            }
            if (creature->IsBanker())
            {
                _needsTown = true;
                _serviceGuid = object.guid;
                _serviceType = "bank";
                _player->GetMotionMaster()->MovePoint(0, object.position.x, object.position.y, object.position.z, FORCED_MOVEMENT_NONE, 0.0f, object.position.orientation);
                return;
            }
            if (creature->IsInnkeeper())
            {
                _needsTown = true;
                _serviceGuid = object.guid;
                _serviceType = "inn";
                _player->GetMotionMaster()->MovePoint(0, object.position.x, object.position.y, object.position.z, FORCED_MOVEMENT_NONE, 0.0f, object.position.orientation);
                return;
            }
        }
    }
}
