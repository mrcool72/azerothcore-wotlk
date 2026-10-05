#include "AutonomousTravelManager.h"
#include "MotionMaster.h"
#include "Player.h"

namespace AutonomousAI
{
    AutonomousTravelManager::AutonomousTravelManager(Player* player) :
        _player(player), _timer(0), _traveling(false), _destination(), _reason("none")
    {
    }

    void AutonomousTravelManager::MoveTo(Position const& position)
    {
        if (!_player || position.mapId != _player->GetMapId())
            return;
        _destination = position;
        _player->GetMotionMaster()->MovePoint(0, position.x, position.y, position.z, FORCED_MOVEMENT_NONE, 0.0f, position.orientation);
        _traveling = true;
    }

    void AutonomousTravelManager::Update(uint32 diff, Perception const& perception, bool externalActive)
    {
        if (!_player || !_player->IsInWorld() || !_player->IsAlive())
            return;
        if (_timer > diff) { _timer -= diff; return; }
        _timer = 3000;

        if (externalActive)
            return;

        // Dungeon navigation is delegated to the dungeon manager. This manager
        // handles ordinary world travel and keeps the movement layer simple.
        if (perception.inDungeon)
        {
            _traveling = false;
            _reason = "dungeon";
            return;
        }

        if (perception.worldRecommendedQuest)
        {
            for (QuestInfo const& quest : perception.quests)
            {
                if (quest.questId == perception.worldRecommendedQuest && quest.giverGuid && quest.giverDistance > 5.0f)
                {
                    for (WorldObjectInfo const& object : perception.nearbyCreatures)
                        if (object.guid == quest.giverGuid) { MoveTo(object.position); _reason = "quest_giver"; return; }
                    for (WorldObjectInfo const& object : perception.nearbyGameObjects)
                        if (object.guid == quest.giverGuid) { MoveTo(object.position); _reason = "quest_object"; return; }
                }
            }
        }

        _traveling = false;
        _reason = "local";
    }
}
