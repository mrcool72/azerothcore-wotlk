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
