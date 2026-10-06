#ifndef TRINITY_AUTONOMOUS_PLAYERBOT_BRIDGE_H
#define TRINITY_AUTONOMOUS_PLAYERBOT_BRIDGE_H

#include <cstdint>

class Player;

namespace AutonomousAI::PlayerbotBridge
{
    // Returns true when mod-playerbots has registered a PlayerbotAI for this player.
    bool IsAvailable(Player* player);

    // Give Playerbot the requested target and let its combat engine choose the
    // appropriate attack/spell/movement action for the bot's class and role.
    bool EngageTarget(Player* player, std::uint64_t targetGuid);
    bool StartQuest(Player* player, std::uint32_t questId);
    bool AcceptAvailableQuests(Player* player);
    bool CompleteQuest(Player* player, std::uint32_t questId);
    bool ContinueQuestWork(Player* player);
    bool ContinueTravel(Player* player);
}

#endif
