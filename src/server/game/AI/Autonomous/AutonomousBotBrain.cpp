#include "AutonomousAcoreCompat.h"
#include "AutonomousBotBrain.h"

#include "Creature.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "QuestDef.h"
#include "Util.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace AutonomousAI
{
    namespace
    {
        constexpr float OBJECTIVE_SEARCH_RANGE = 55.0f;
        constexpr float INTERACTION_RANGE = 4.0f;
        constexpr float EXPLORE_RADIUS = 35.0f;
        constexpr uint32 RECOVERY_ENTER_PCT = 45;
        constexpr uint32 RECOVERY_EXIT_PCT = 80;

        bool MatchesObjective(uint32 entry, uint32 required)
        {
            if (!required)
                return false;

            if (entry == required)
                return true;

            // Quest templates use a negative RequiredNpcOrGo value to identify a
            // gameobject. The protocol stores the value as uint32, so compare the
            // wrapped representation as well.
            return entry == static_cast<uint32>(-static_cast<int32>(required));
        }

        bool HasUnfinishedObjective(QuestObjectiveInfo const& quest, uint8 index)
        {
            if (index >= quest.requiredCreatureOrGameObject.size() ||
                index >= quest.creatureOrGameObjectProgress.size())
                return false;

            uint32 required = quest.requiredCreatureOrGameObject[index];
            uint32 progress = quest.creatureOrGameObjectProgress[index];
            if (!required)
                return false;

            if (index < quest.requiredCreatureOrGameObjectCount.size())
            {
                uint32 requiredCount = quest.requiredCreatureOrGameObjectCount[index];
                if (requiredCount && progress >= requiredCount)
                    return false;
            }

            return true;
        }
    }

    AutonomousBotBrain::AutonomousBotBrain(Player* player) :
        _player(player),
        _enabled(true),
        _thinkTimer(0),
        _exploreTimer(0),
        _questRequest(0),
        _personality("balanced"),
        _recovering(false)
    {
    }

    void AutonomousBotBrain::Update(uint32 diff, Perception const& perception, bool questExecutorActive)
    {
        if (!_enabled || !_player || !_player->IsInWorld())
            return;

        if (_recovering)
        {
            if (_player->IsAlive() && _player->GetHealthPct() >= RECOVERY_EXIT_PCT)
            {
                _recovering = false;
                _thinkTimer = 500;
            }
            else
            {
                _player->GetMotionMaster()->Clear();
                return;
            }
        }

        if (_player->IsAlive() && _player->GetHealthPct() <= RECOVERY_ENTER_PCT)
        {
            _recovering = true;
            Recover();
            return;
        }

        if (_thinkTimer > diff)
        {
            _thinkTimer -= diff;
            return;
        }
        _thinkTimer = 1000;

        if (questExecutorActive)
        {
            WorkOnQuest(perception);
            return;
        }

        // Dungeon parties are coordinated by the social/combat managers. Do not
        // let the leader's generic exploration behavior pull unrelated packs.
        if (perception.inDungeon && perception.groupMembers.size() > 1)
            return;

        if (SelectQuest(perception))
            return;

        if (_personality == "quester")
            return;

        if (_exploreTimer > diff)
            _exploreTimer -= diff;
        else
        {
            _exploreTimer = 5000;
            Explore();
        }
    }

    bool AutonomousBotBrain::SelectQuest(Perception const& perception)
    {
        uint32 chainQuestId = 0;
        for (QuestObjectiveInfo const& active : perception.activeQuests)
            if (!active.complete) { chainQuestId = active.questId; break; }

        QuestInfo const* best = nullptr;
        float bestScore = std::numeric_limits<float>::max();
        for (QuestInfo const& quest : perception.quests)
        {
            if (!quest.canAccept || !quest.giverGuid)
                continue;

            float score = quest.giverDistance;
            if (chainQuestId && (quest.previousQuestId == static_cast<int32>(chainQuestId) ||
                quest.previousQuestId == -static_cast<int32>(chainQuestId)))
                score -= 250.0f;
            if (perception.worldRecommendedQuest == quest.questId)
                score -= 125.0f;
            if (_personality == "quester")
                score *= 0.55f;
            else if (_personality == "explorer")
                score *= 1.75f;
            else if (_personality == "cautious")
                score *= 1.15f;

            if (!best || score < bestScore)
            {
                best = &quest;
                bestScore = score;
            }
        }

        if (!best)
            return false;

        // The controller's quest executor performs the authoritative quest
        // interaction and movement. The brain only chooses the next goal.
        _questRequest = best->questId;
        return true;
    }

    uint32 AutonomousBotBrain::ConsumeQuestRequest()
    {
        uint32 questId = _questRequest;
        _questRequest = 0;
        return questId;
    }

    bool AutonomousBotBrain::WorkOnQuest(Perception const& perception)
    {
        QuestObjectiveInfo const* active = nullptr;
        for (QuestObjectiveInfo const& quest : perception.activeQuests)
        {
            if (!quest.complete)
            {
                active = &quest;
                break;
            }
        }

        if (!active)
            return false;

        for (uint8 objective = 0; objective < QUEST_OBJECTIVES_COUNT; ++objective)
        {
            if (!HasUnfinishedObjective(*active, objective))
                continue;

            uint32 required = active->requiredCreatureOrGameObject[objective];
            for (WorldObjectInfo const& object : perception.nearbyCreatures)
            {
                if (!object.alive || !MatchesObjective(object.entry, required))
                    continue;

                if (object.distance > INTERACTION_RANGE)
                {
                    MoveTo(object.position);
                    return true;
                }

                if (Unit* target = ObjectAccessor::GetUnit(*_player, AutonomousMakeGuid(object.guid)))
                {
                    if (_player->IsValidAttackTarget(target))
                        _player->Attack(target, true);
                    return true;
                }
            }

            for (WorldObjectInfo const& object : perception.nearbyGameObjects)
            {
                if (!MatchesObjective(object.entry, required))
                    continue;

                if (object.distance > INTERACTION_RANGE)
                {
                    MoveTo(object.position);
                    return true;
                }

                MoveTo(object.position);
                return true;
            }

            // Item objectives do not expose their drop source through the quest
            // template. If an item objective is still incomplete, safely hunt a
            // nearby hostile creature instead of standing idle; TrinityCore still
            // decides whether the kill awards the item.
            if (objective < active->requiredItems.size() &&
                objective < active->requiredItemCount.size() &&
                objective < active->itemProgress.size() &&
                active->requiredItems[objective] &&
                active->itemProgress[objective] < active->requiredItemCount[objective])
            {
                WorldObjectInfo const* target = nullptr;
                for (WorldObjectInfo const& creature : perception.nearbyCreatures)
                {
                    if (!creature.alive || !creature.hostile)
                        continue;
                    if (!target || creature.distance < target->distance)
                        target = &creature;
                }
                if (target)
                {
                    if (target->distance > INTERACTION_RANGE)
                        MoveTo(target->position);
                    else if (Unit* unit = ObjectAccessor::GetUnit(*_player, AutonomousMakeGuid(target->guid)))
                        if (_player->IsValidAttackTarget(unit))
                            _player->Attack(unit, true);
                    return true;
                }
            }

            // Nothing matching the objective is currently visible. Give the bot
            // a small local search pattern; authoritative movement/pathfinding
            // remains inside TrinityCore.
            Explore();
            return true;
        }

        return false;
    }

    bool AutonomousBotBrain::Recover()
    {
        if (!_player || !_player->IsAlive())
            return false;

        _player->GetMotionMaster()->Clear();
        return true;
    }

    void AutonomousBotBrain::MoveTo(Position const& position)
    {
        if (!_player || position.mapId != _player->GetMapId())
            return;

        _player->GetMotionMaster()->MovePoint(0, position.x, position.y, position.z, FORCED_MOVEMENT_NONE, 0.0f, position.orientation);
    }

    void AutonomousBotBrain::Explore()
    {
        if (!_player)
            return;

        float radius = EXPLORE_RADIUS;
        if (_personality == "explorer")
            radius = 70.0f;
        else if (_personality == "cautious")
            radius = 20.0f;

        ::Position worldPosition = _player->GetRandomPoint(_player->GetPosition(), radius);
        Position destination;
        destination.mapId = _player->GetMapId();
        destination.x = worldPosition.GetPositionX();
        destination.y = worldPosition.GetPositionY();
        destination.z = worldPosition.GetPositionZ();
        destination.orientation = worldPosition.GetOrientation();
        MoveTo(destination);
    }
}
