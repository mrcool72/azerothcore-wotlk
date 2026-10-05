#ifndef TRINITY_AUTONOMOUS_HEADLESS_PLAYER_H
#define TRINITY_AUTONOMOUS_HEADLESS_PLAYER_H

#include "ObjectGuid.h"
#include "Common.h"
#include <functional>
#include <string>

class Player;
class WorldSession;

namespace AutonomousAI
{
    using HeadlessPlayerCallback = std::function<void(Player*, WorldSession*)>;

    void LoadHeadlessPlayer(ObjectGuid guid, HeadlessPlayerCallback callback);
    void UnloadHeadlessPlayer(WorldSession* session);

    // Creates a persistent character using TrinityCore's normal Player::Create/SaveToDB path.
    // The resulting character is not automatically inserted into the world.
    bool CreateAutonomousCharacter(uint32 accountId, std::string const& name, uint8 race, uint8 classId,
        uint8 gender, ObjectGuid& outGuid);
}

#endif
