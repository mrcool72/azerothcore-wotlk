#include "AutonomousCampaignNavigationManager.h"
#include "AutonomousQuestKnowledgeManager.h"


namespace AutonomousAI
{
namespace
{
    bool IsSameGuid(uint64 lhs, uint64 rhs) { return lhs != 0 && lhs == rhs; }
}

void AutonomousCampaignNavigationManager::ClearAction()
{
    _hasAction = false;
    _action = Action();
}

Action AutonomousCampaignNavigationManager::ConsumeAction()
{
    Action result = _action;
    ClearAction();
    return result;
}

void AutonomousCampaignNavigationManager::SelectObjectiveAction(Perception const& perception)
{
    if (!perception.campaignTargetGuid)
    {
        _state = perception.campaignExecutorState == "objective" ? "searching_objective" : "no_target";
        return;
    }

    WorldObjectInfo const* best = nullptr;

    for (WorldObjectInfo const& object : perception.nearbyCreatures)
    {
        if (!IsSameGuid(object.guid, perception.campaignTargetGuid) || !object.alive)
            continue;
        best = &object;
        break;
    }

    if (!best)
    {
        for (WorldObjectInfo const& object : perception.nearbyGameObjects)
        {
            if (!IsSameGuid(object.guid, perception.campaignTargetGuid))
                continue;
            best = &object;
            break;
        }
    }

    if (!best)
    {
        _state = "target_not_visible";
        return;
    }

    float bestDistance = best->distance;
    if (bestDistance > 4.0f)
    {
        _action.type = ActionType::MOVE_TO;
        _action.targetGuid = best->guid;
        _action.entry = best->entry;
        _action.destination = best->position;
        _hasAction = true;
        _state = "travel_to_objective";
        return;
    }

    if (best->hostile && best->alive)
    {
        _action.type = ActionType::ATTACK;
        _action.targetGuid = best->guid;
        _action.entry = best->entry;
        _hasAction = true;
        _state = "attack_objective";
        return;
    }

    _state = "at_objective";
}

void AutonomousCampaignNavigationManager::Update(uint32 diff, Perception const& perception, bool externalActive)
{
    ClearAction();
    if (_timer > diff)
    {
        _timer -= diff;
        return;
    }
    _timer = 1000;

    if (externalActive)
    {
        _state = "external";
        return;
    }

    if (!perception.campaignQuest)
    {
        _state = "idle";
        _searchTicks = 0;
        return;
    }

    if (perception.campaignStalled)
    {
        _state = "stalled";
        return;
    }

    if (perception.campaignExecutorState == "objective" || perception.campaignTargetGuid)
    {
        SelectObjectiveAction(perception);
        if (!_hasAction && perception.campaignKnowledgeKnown && perception.campaignKnowledgeMap == perception.mapId)
        {
            _action.type = ActionType::MOVE_TO;
            _action.destination = perception.campaignKnowledgePosition;
            _action.entry = perception.campaignTargetEntry;
            _hasAction = true;
            _state = "travel_to_known_objective";
        }
        if (_state == "searching_objective" || _state == "target_not_visible")
            ++_searchTicks;
        else
            _searchTicks = 0;
        return;
    }

    _searchTicks = 0;
    if (perception.campaignExecutorState == "starting" || perception.campaignState == "selected")
    {
        // The quest executor owns quest-giver interaction. We only report that
        // the campaign is waiting for its authoritative state transition.
        _state = "waiting_quest_executor";
        return;
    }

    _state = perception.campaignExecutorState;
}
}
