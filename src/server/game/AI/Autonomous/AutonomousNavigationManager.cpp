#include "AutonomousNavigationManager.h"

namespace AutonomousAI
{
    void AutonomousNavigationManager::Update(uint32 diff, Perception const& perception)
    {
        if (_timer > diff)
        {
            _timer -= diff;
            return;
        }
        _timer = 2500;

        _hasPlan = false;
        _stage = "idle";
        _reason = "none";
        _destination = Position();

        if (perception.inDungeon)
        {
            if (perception.dungeonRegroupRequired)
            {
                _stage = "regroup";
                _reason = "dungeon_regroup";
            }
            else if (perception.dungeonNextBossGuid)
            {
                _stage = "dungeon_objective";
                _reason = "next_boss";
            }
            else
            {
                _stage = "dungeon_progress";
                _reason = "dungeon";
            }
            ++_step;
            return;
        }

        if (perception.needsTown)
        {
            _hasPlan = true;
            _stage = "town";
            _reason = perception.townServiceType.empty() ? "inventory_pressure" : perception.townServiceType;
            _destination = perception.position;
            ++_step;
            return;
        }

        if (perception.worldRecommendedQuest)
        {
            _hasPlan = true;
            _stage = "quest_chain";
            _reason = perception.activeQuests.empty() ? "recommended_quest" : "active_quest";
            for (WorldObjectInfo const& object : perception.nearbyCreatures)
            {
                if (object.questGiver && object.distance <= 80.0f)
                {
                    _destination = object.position;
                    break;
                }
            }
            ++_step;
            return;
        }

        if (perception.preferredCompanionGuid && perception.socialScore >= 5)
        {
            _stage = "social";
            _reason = "preferred_companion";
            ++_step;
            return;
        }

        _stage = "exploration";
        _reason = "no_active_objective";
        ++_step;
    }
}
