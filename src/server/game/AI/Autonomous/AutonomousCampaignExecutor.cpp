#include "AutonomousCampaignExecutor.h"

#include "AutonomousBotController.h"
#include "AutonomousBotMgr.h"
#include "Creature.h"
#include "Player.h"

#include <algorithm>
#include <limits>

namespace AutonomousAI
{
namespace
{
    bool MatchesObjective(uint32 entry, uint32 required)
    {
        if (!required)
            return false;
        if (entry == required)
            return true;
        return entry == static_cast<uint32>(-static_cast<int32>(required));
    }

    uint32 ProgressValue(QuestObjectiveInfo const& quest)
    {
        uint32 value = 0;
        for (uint32 n : quest.creatureOrGameObjectProgress)
            value += n;
        for (uint32 n : quest.itemProgress)
            value += n;
        return value;
    }

    bool HasIncompleteObjective(QuestObjectiveInfo const& quest)
    {
        if (quest.complete)
            return false;

        for (size_t i = 0; i < quest.requiredCreatureOrGameObject.size(); ++i)
        {
            uint32 required = quest.requiredCreatureOrGameObject[i];
            uint32 current = i < quest.creatureOrGameObjectProgress.size() ? quest.creatureOrGameObjectProgress[i] : 0;
            uint32 needed = i < quest.requiredCreatureOrGameObjectCount.size() ? quest.requiredCreatureOrGameObjectCount[i] : 0;
            if (required && (!needed || current < needed))
                return true;
        }

        for (size_t i = 0; i < quest.requiredItems.size(); ++i)
        {
            uint32 required = quest.requiredItems[i];
            uint32 current = i < quest.itemProgress.size() ? quest.itemProgress[i] : 0;
            uint32 needed = i < quest.requiredItemCount.size() ? quest.requiredItemCount[i] : 0;
            if (required && (!needed || current < needed))
                return true;
        }

        return quest.explored == false;
    }
}

AutonomousCampaignExecutor::AutonomousCampaignExecutor(Player* player) :
    _player(player)
{
}

bool AutonomousCampaignExecutor::HasProgress(QuestObjectiveInfo const& quest) const
{
    return ProgressValue(quest) > _lastProgress;
}

void AutonomousCampaignExecutor::ResetProgress(QuestObjectiveInfo const& quest)
{
    _lastProgress = ProgressValue(quest);
}

void AutonomousCampaignExecutor::ResolveTarget(Perception const& perception)
{
    _targetGuid = 0;
    _targetEntry = 0;

    QuestObjectiveInfo const* active = nullptr;
    for (QuestObjectiveInfo const& quest : perception.activeQuests)
    {
        if (quest.questId == _campaignQuest && !quest.complete)
        {
            active = &quest;
            break;
        }
    }

    if (!active || !HasIncompleteObjective(*active))
        return;

    // Preserve an objective entry even when no matching object is currently
    // visible. This lets shared world knowledge provide a remembered location.
    for (size_t i = 0; i < active->requiredCreatureOrGameObject.size(); ++i)
    {
        uint32 required = active->requiredCreatureOrGameObject[i];
        uint32 progress = i < active->creatureOrGameObjectProgress.size() ? active->creatureOrGameObjectProgress[i] : 0;
        uint32 needed = i < active->requiredCreatureOrGameObjectCount.size() ? active->requiredCreatureOrGameObjectCount[i] : 0;
        if (required && (!needed || progress < needed))
        {
            _targetEntry = required < 0x80000000u ? required : static_cast<uint32>(-static_cast<int32>(required));
            break;
        }
    }

    float bestDistance = std::numeric_limits<float>::max();

    for (size_t i = 0; i < active->requiredCreatureOrGameObject.size(); ++i)
    {
        uint32 required = active->requiredCreatureOrGameObject[i];
        uint32 progress = i < active->creatureOrGameObjectProgress.size() ? active->creatureOrGameObjectProgress[i] : 0;
        uint32 needed = i < active->requiredCreatureOrGameObjectCount.size() ? active->requiredCreatureOrGameObjectCount[i] : 0;
        if (!required || (needed && progress >= needed))
            continue;

        for (WorldObjectInfo const& object : perception.nearbyCreatures)
        {
            if (!object.alive || !MatchesObjective(object.entry, required))
                continue;
            if (object.distance < bestDistance)
            {
                bestDistance = object.distance;
                _targetGuid = object.guid;
                _targetEntry = object.entry;
            }
        }

        for (WorldObjectInfo const& object : perception.nearbyGameObjects)
        {
            if (!MatchesObjective(object.entry, required))
                continue;
            if (object.distance < bestDistance)
            {
                bestDistance = object.distance;
                _targetGuid = object.guid;
                _targetEntry = object.entry;
            }
        }
    }

    if (_targetGuid)
        return;

    for (size_t i = 0; i < active->requiredItems.size(); ++i)
    {
        uint32 required = active->requiredItems[i];
        uint32 progress = i < active->itemProgress.size() ? active->itemProgress[i] : 0;
        uint32 needed = i < active->requiredItemCount.size() ? active->requiredItemCount[i] : 0;
        if (!required || (needed && progress >= needed))
            continue;

        for (WorldObjectInfo const& object : perception.nearbyCreatures)
        {
            if (!object.alive || !object.hostile)
                continue;
            if (object.distance < bestDistance)
            {
                bestDistance = object.distance;
                _targetGuid = object.guid;
                _targetEntry = object.entry;
            }
        }
    }
}

void AutonomousCampaignExecutor::Update(uint32 diff, Perception const& perception,
    uint32 campaignQuest, bool questExecutorActive, bool externalActive)
{
    if (!_player || !_player->IsInWorld())
        return;

    if (_retryTimer > diff)
        _retryTimer -= diff;
    else
        _retryTimer = 0;

    if (_thinkTimer > diff)
    {
        _thinkTimer -= diff;
        return;
    }
    _thinkTimer = 1000;

    if (campaignQuest != _campaignQuest)
    {
        _campaignQuest = campaignQuest;
        _targetGuid = 0;
        _targetEntry = 0;
        _stallTimer = 0;
        _retryCount = 0;
        _stalled = false;
        _state = _campaignQuest ? "selected" : "idle";
        for (QuestObjectiveInfo const& quest : perception.activeQuests)
            if (quest.questId == _campaignQuest)
                ResetProgress(quest);
    }

    if (!_campaignQuest)
    {
        _state = "idle";
        _targetGuid = 0;
        _targetEntry = 0;
        return;
    }

    if (questExecutorActive)
    {
        _state = "quest_executor";
        _stalled = false;

        for (QuestObjectiveInfo const& quest : perception.activeQuests)
        {
            if (quest.questId != _campaignQuest)
                continue;

            if (HasProgress(quest))
            {
                ResetProgress(quest);
                _stallTimer = 0;
            }
            else if (HasIncompleteObjective(quest))
            {
                _stallTimer += 1000;
            }
            break;
        }

        ResolveTarget(perception);

        if (_stallTimer >= 45000)
        {
            _stalled = true;
            _state = "stalled";
            ++_retryCount;
            _retryTimer = std::min<uint32>(60000, 5000u * _retryCount);
            _stallTimer = 0;
            if (AutonomousBotController* controller = sAutonomousBotMgr.Find(_player->GetGUID().GetRawValue()))
                controller->CancelQuest();
        }
        return;
    }

    _targetGuid = 0;
    _targetEntry = 0;

    if (externalActive)
    {
        _state = "waiting_external";
        return;
    }

    if (_retryTimer)
    {
        _state = _stalled ? "retry_wait" : "waiting";
        return;
    }

    // The campaign selector only exposes quests that are visible/valid to the bot.
    // Reuse the authoritative quest executor instead of duplicating quest state changes.
    if (_player->IsActiveQuest(_campaignQuest))
    {
        _state = "objective";
        ResolveTarget(perception);
        return;
    }

    AutonomousBotController* controller = sAutonomousBotMgr.Find(_player->GetGUID().GetRawValue());
    if (controller && controller->StartQuest(_campaignQuest))
    {
        _state = "starting";
        _stallTimer = 0;
        return;
    }

    ++_retryCount;
    _retryTimer = std::min<uint32>(60000, 5000u * _retryCount);
    _state = "start_failed";
}
}
