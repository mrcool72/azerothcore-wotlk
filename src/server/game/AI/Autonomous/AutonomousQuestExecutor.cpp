#include "AutonomousAcoreCompat.h"
#include "AutonomousQuestExecutor.h"
#include "AutonomousBotProtocol.h"

#include "GameObject.h"
#include "MotionMaster.h"
#include "Player.h"
#include "QuestDef.h"
#include "ObjectMgr.h"

#include <limits>

namespace AutonomousAI
{
    AutonomousQuestExecutor::AutonomousQuestExecutor(Player* player) :
        _player(player), _questId(0), _state(State::IDLE), _thinkTimer(0), _giverGuid(0)
    {
    }

    char const* AutonomousQuestExecutor::GetStateName() const
    {
        switch (_state)
        {
            case State::FIND_GIVER: return "find_giver";
            case State::TRAVEL_TO_GIVER: return "travel_to_giver";
            case State::ACCEPT: return "accept";
            case State::EXECUTE: return "execute";
            case State::RETURN_TO_GIVER: return "return_to_giver";
            case State::TURN_IN: return "turn_in";
            case State::DONE: return "done";
            default: return "idle";
        }
    }

    bool AutonomousQuestExecutor::Start(uint32 questId)
    {
        if (!_player || !questId || !sObjectMgr->GetQuestTemplate(questId))
            return false;

        _questId = questId;
        _giverGuid = 0;
        _state = State::FIND_GIVER;
        _thinkTimer = 0;
        return true;
    }

    void AutonomousQuestExecutor::Cancel()
    {
        _questId = 0;
        _giverGuid = 0;
        _state = State::IDLE;
        _thinkTimer = 0;
    }

    bool AutonomousQuestExecutor::FindGiver(Perception const& perception, uint64& guid, Position& position, bool involved) const
    {
        float best = std::numeric_limits<float>::max();
    auto hasCreatureQuest = [&](uint32 entry, bool involved)
    {
        if (involved)
        {
            auto bounds = sObjectMgr->GetCreatureQuestInvolvedRelationBounds(entry);
            return std::any_of(bounds.first, bounds.second,
                [&](auto const& relation) { return relation.second == _questId; });
        }

        auto bounds = sObjectMgr->GetCreatureQuestRelationBounds(entry);
        return std::any_of(bounds.first, bounds.second,
            [&](auto const& relation) { return relation.second == _questId; });
    };

    auto hasGameObjectQuest = [&](uint32 entry, bool involved)
    {
        if (involved)
        {
            auto bounds = sObjectMgr->GetGOQuestInvolvedRelationBounds(entry);
            return std::any_of(bounds.first, bounds.second,
                [&](auto const& relation) { return relation.second == _questId; });
        }

        auto bounds = sObjectMgr->GetGOQuestRelationBounds(entry);
        return std::any_of(bounds.first, bounds.second,
            [&](auto const& relation) { return relation.second == _questId; });
    };
        for (WorldObjectInfo const& object : perception.nearbyCreatures)
        {
            if (!object.questGiver)
                continue;

            bool related = false;
            if (involved)
                related = hasCreatureQuest(object.entry, true);
            else
                related = hasCreatureQuest(object.entry, false);

            if (related && object.distance < best)
            {
                best = object.distance;
                guid = object.guid;
                position = object.position;
            }
        }

        for (WorldObjectInfo const& object : perception.nearbyGameObjects)
        {
            bool related = false;
            if (involved)
                related = hasGameObjectQuest(object.entry, true);
            else
                related = hasGameObjectQuest(object.entry, false);

            if (related && object.distance < best)
            {
                best = object.distance;
                guid = object.guid;
                position = object.position;
            }
        }

        return guid != 0;
    }

    void AutonomousQuestExecutor::MoveTo(Position const& position)
    {
        if (!_player || position.mapId != _player->GetMapId())
            return;

        _player->GetMotionMaster()->MovePoint(0, position.x, position.y, position.z, FORCED_MOVEMENT_NONE, 0.0f, position.orientation);
    }

    bool AutonomousQuestExecutor::Interact(uint64 guid, bool reward)
    {
        if (!_player || !guid)
            return false;

        ObjectGuid objectGuid = AutonomousMakeGuid(guid);
        Object* giver = nullptr;
        if (Creature* creature = _player->GetNPCIfCanInteractWith(objectGuid, UNIT_NPC_FLAG_QUESTGIVER))
            giver = creature;
        else if (GameObject* gameObject = _player->GetGameObjectIfCanInteractWith(objectGuid, GAMEOBJECT_TYPE_QUESTGIVER))
            giver = gameObject;

        if (!giver)
            return false;

        Quest const* quest = sObjectMgr->GetQuestTemplate(_questId);
        if (!quest)
            return false;

        if (!reward)
        {
            if (_player->CanTakeQuest(quest, false) && _player->CanAddQuest(quest, false))
            {
                _player->AddQuestAndCheckCompletion(quest, giver);
                return true;
            }
            return _player->IsActiveQuest(_questId);
        }

        if (_player->CanRewardQuest(quest, 0, false))
        {
            _player->RewardQuest(quest, 0, giver);
            return true;
        }
        return false;
    }

    void AutonomousQuestExecutor::Update(uint32 diff, Perception const& perception)
    {
        if (!_player || !_questId)
            return;

        if (_thinkTimer > diff)
        {
            _thinkTimer -= diff;
            return;
        }
        _thinkTimer = 750;

        Quest const* quest = sObjectMgr->GetQuestTemplate(_questId);
        if (!quest)
        {
            Cancel();
            return;
        }

        if (!_player->IsActiveQuest(_questId) && _player->GetQuestStatus(_questId) == QUEST_STATUS_NONE)
            _state = State::FIND_GIVER;

        switch (_state)
        {
            case State::FIND_GIVER:
            {
                Position position;
                if (FindGiver(perception, _giverGuid, position, false))
                {
                    if (_player->GetDistance(position.x, position.y, position.z) <= 4.0f)
                        _state = State::ACCEPT;
                    else
                    {
                        MoveTo(position);
                        _state = State::TRAVEL_TO_GIVER;
                    }
                }
                break;
            }
            case State::TRAVEL_TO_GIVER:
                if (Interact(_giverGuid, false))
                    _state = State::EXECUTE;
                break;
            case State::ACCEPT:
                if (Interact(_giverGuid, false))
                    _state = State::EXECUTE;
                break;
            case State::EXECUTE:
                if (_player->CanCompleteQuest(_questId))
                    _state = State::RETURN_TO_GIVER;
                break;
            case State::RETURN_TO_GIVER:
            {
                Position position;
                if (FindGiver(perception, _giverGuid, position, true))
                {
                    if (_player->GetDistance(position.x, position.y, position.z) <= 4.0f)
                        _state = State::TURN_IN;
                    else
                        MoveTo(position);
                }
                break;
            }
            case State::TURN_IN:
                if (Interact(_giverGuid, true))
                {
                    _state = State::DONE;
                    _questId = 0;
                }
                break;
            case State::DONE:
                Cancel();
                break;
            default:
                break;
        }
    }
}
