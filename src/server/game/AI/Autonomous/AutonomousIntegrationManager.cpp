#include "AutonomousAcoreCompat.h"
#include "AutonomousIntegrationManager.h"
#include "AutonomousBotProtocol.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "GameObject.h"

namespace AutonomousAI
{
    void AutonomousIntegrationManager::Update(uint32 diff, Player* player, Perception const& perception, bool externalActive)
    {
        if (_timer > diff)
        {
            _timer -= diff;
            return;
        }
        _timer = 1200;
        _hasAction = false;
        _action = Action();
        _decision = "idle";

        if (!player || externalActive || !player->IsInWorld() || !player->IsAlive() || player->IsInCombat())
            return;

        if (perception.inDungeon)
            return;

        if (perception.needsTown || perception.economyNeedsAttention)
            return;

        if (perception.resourceCandidateGuid && perception.resourceCandidateDistance > 6.0f)
        {
            if (GameObject* object = ObjectAccessor::GetGameObject(*player, [&]() { ObjectGuid g; g = AutonomousMakeGuid(perception.resourceCandidateGuid); return g; }()))
            {
                _action.type = ActionType::MOVE_TO;
                auto const& worldPosition = object->GetPosition();
                _action.destination.mapId = player->GetMapId();
                _action.destination.x = worldPosition.GetPositionX();
                _action.destination.y = worldPosition.GetPositionY();
                _action.destination.z = worldPosition.GetPositionZ();
                _action.destination.orientation = worldPosition.GetOrientation();
                _decision = "resource_object";
                _hasAction = true;
            }
        }

        if (!_hasAction && perception.preferredCompanionGuid && perception.socialScore >= 5)
        {
            if (Player* companion = ObjectAccessor::FindPlayer([&]() { ObjectGuid g; g = AutonomousMakeGuid(perception.preferredCompanionGuid); return g; }()))
            {
                if (companion->GetMapId() == player->GetMapId() && companion != player && companion->IsInWorld() && companion->IsAlive())
                {
                    player->GetMotionMaster()->MoveFollow(companion, 5.0f, 0.0f);
                    _decision = "follow_companion";
                }
            }
        }

        if (!_hasAction && perception.navigationHasPlan && perception.navigationDestination.mapId == player->GetMapId())
        {
            Position const& destination = perception.navigationDestination;
            if (destination.x != 0.0f || destination.y != 0.0f || destination.z != 0.0f)
            {
                _action.type = ActionType::MOVE_TO;
                _action.destination = destination;
                _decision = perception.navigationStage.empty() ? "navigation" : perception.navigationStage;
                _hasAction = true;
            }
        }

        if (_hasAction || _decision != "idle")
            ++_decisionCount;
    }
}
