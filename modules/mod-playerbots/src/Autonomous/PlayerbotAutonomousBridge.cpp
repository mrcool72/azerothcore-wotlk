#include "AutonomousPlayerbotBridge.h"

#include "ObjectAccessor.h"
#include "Player.h"
#include "PlayerbotAI.h"
#include "PlayerbotMgr.h"
#include "Playerbots.h"
#include "Unit.h"

namespace AutonomousAI::PlayerbotBridge
{
    bool IsAvailable(Player* player)
    {
        return player && sPlayerbotsMgr.GetPlayerbotAI(player) != nullptr;
    }

    bool StartQuest(Player* player, std::uint32_t questId)
    {
        if (!IsAvailable(player) || !questId)
            return false;

        std::string response = sPlayerbotsMgr.GetPlayerbotAI(player)->HandleRemoteCommand(
            "rpg do quest " + std::to_string(questId));
        return !response.empty();
    }

    bool AcceptAvailableQuests(Player* player)
    {
        if (!IsAvailable(player))
            return false;

        std::string response = sPlayerbotsMgr.GetPlayerbotAI(player)->HandleRemoteCommand("quest fetch");
        return !response.empty();
    }

    bool CompleteQuest(Player* player, std::uint32_t questId)
    {
        if (!IsAvailable(player) || !questId)
            return false;

        std::string response = sPlayerbotsMgr.GetPlayerbotAI(player)->HandleRemoteCommand(
            "quest complete " + std::to_string(questId));
        return !response.empty();
    }

    bool ContinueQuestWork(Player* player)
    {
        if (!IsAvailable(player))
            return false;

        PlayerbotAI* botAI = sPlayerbotsMgr.GetPlayerbotAI(player);
        return botAI->DoSpecificAction("new rpg do quest", Event(), true);
    }

    bool ContinueTravel(Player* player)
    {
        if (!IsAvailable(player))
            return false;

        PlayerbotAI* botAI = sPlayerbotsMgr.GetPlayerbotAI(player);
        bool choseTarget = botAI->DoSpecificAction("choose travel target", Event(), true);
        bool traveled = botAI->DoSpecificAction("travel", Event(), true);
        return choseTarget || traveled;
    }

    bool EngageTarget(Player* player, std::uint64_t targetGuid)
    {
        if (!player || !targetGuid)
            return false;

        PlayerbotAI* botAI = sPlayerbotsMgr.GetPlayerbotAI(player);
        if (!botAI)
            return false;

        ObjectGuid guid(targetGuid);
        Unit* target = ObjectAccessor::GetUnit(*player, guid);
        if (!target || !target->IsAlive() || !player->IsValidAttackTarget(target))
            return false;

        AiObjectContext* context = botAI->GetAiObjectContext();
        if (!context)
            return false;

        context->GetValue<Unit*>("current target")->Set(target);
        context->GetValue<GuidVector>("prioritized targets")->Set({ target->GetGUID() });
        player->SetSelection(target->GetGUID());

        botAI->ChangeEngine(BOT_STATE_COMBAT);
        botAI->DoNextAction(false);
        return true;
    }
}
