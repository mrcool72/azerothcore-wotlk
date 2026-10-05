#include "ScriptMgr.h"
#include "Chat.h"
#include "ChatCommand.h"
#include "Player.h"
#include "../../game/Server/WorldSession.h"
#include "WorldSession.h"
#include "ObjectAccessor.h"
#include "RBAC.h"

#include "AutonomousBotMgr.h"
#include "AutonomousBotController.h"
#include "AutonomousBotProfile.h"
#include "AutonomousHeadlessPlayer.h"

#include <charconv>
#include <sstream>
#include <span>

using namespace Acore::ChatCommands;

class autonomousbot_commandscript : public CommandScript
{
public:
    autonomousbot_commandscript() : CommandScript("autonomousbot_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable botCommandTable =
        {
            { "list",   HandleList,   rbac::RBAC_PERM_COMMAND_NPC_INFO, Console::No },
            { "add",    HandleAdd,    rbac::RBAC_PERM_COMMAND_NPC_INFO, Console::No },
            { "create", HandleCreate, rbac::RBAC_PERM_COMMAND_NPC_INFO, Console::Yes },
            { "delete", HandleDelete, rbac::RBAC_PERM_COMMAND_NPC_INFO, Console::No },
            { "attach", HandleAttach, rbac::RBAC_PERM_COMMAND_NPC_INFO, Console::No },
            { "detach", HandleDetach, rbac::RBAC_PERM_COMMAND_NPC_INFO, Console::No },
            { "start",  HandleStart,  rbac::RBAC_PERM_COMMAND_NPC_INFO, Console::No },
            { "stop",   HandleStop,   rbac::RBAC_PERM_COMMAND_NPC_INFO, Console::No },
            { "info",   HandleInfo,   rbac::RBAC_PERM_COMMAND_NPC_INFO, Console::No },
            { "quest",  HandleQuest,  rbac::RBAC_PERM_COMMAND_NPC_INFO, Console::No },
            { "edit",   HandleEdit,   rbac::RBAC_PERM_COMMAND_NPC_INFO, Console::No },
            { "startall", HandleStartAll, rbac::RBAC_PERM_COMMAND_NPC_INFO, Console::Yes },
            { "stopall",  HandleStopAll,  rbac::RBAC_PERM_COMMAND_NPC_INFO, Console::Yes },
        };

        static ChatCommandTable commandTable =
        {
            { "autobot", botCommandTable },
        };
        return commandTable;
    }

private:
    static Player* GetTarget(ChatHandler* handler)
    {
        Player* player = handler->getSelectedPlayer();
        if (!player)
        {
            handler->SendSysMessage("Select an online player first.");
            handler->SetSentErrorMessage(true);
        }
        return player;
    }

    static bool HandleList(ChatHandler* handler)
    {
        auto profiles = AutonomousAI::sAutonomousBotMgr.GetProfiles();
        if (profiles.empty())
        {
            handler->SendSysMessage("No autonomous bot profiles configured.");
            return true;
        }

        for (auto const& profile : profiles)
        {
            Player* player = ObjectAccessor::FindPlayer(AutonomousAI::AutonomousMakeGuid(profile.guid));
            bool online = player != nullptr;
            bool attached = AutonomousAI::sAutonomousBotMgr.Find(profile.guid) != nullptr;
            bool headless = AutonomousAI::sAutonomousBotMgr.IsHeadless(profile.guid);
            handler->PSendSysMessage("%s [%llu] endpoint=%s:%u enabled=%u online=%u attached=%u headless=%u personality=%s role=%s",
                profile.name.c_str(), static_cast<unsigned long long>(profile.guid), profile.host.c_str(), profile.port,
                profile.enabled ? 1 : 0, online ? 1 : 0, attached ? 1 : 0, headless ? 1 : 0, profile.personality.c_str(), profile.role.c_str());
        }
        return true;
    }

    static bool HandleCreate(ChatHandler* handler, Tail text)
    {
        std::istringstream input{ std::string(text) };
        std::string name;
        uint32 race = 0;
        uint32 classId = 0;
        uint32 gender = 0;
        uint32 accountId = handler->GetSession() ? handler->GetSession()->GetAccountId() : 0;

        input >> name >> race >> classId;
        if (name.empty() || race == 0 || classId == 0)
        {
            handler->SendSysMessage("Usage: .autobot create <name> <race> <class> [gender] [accountId]");
            return false;
        }

        if (input >> gender)
            input >> accountId;

        if (!accountId || race > 255 || classId > 255 || gender > 2)
            return false;

        ObjectGuid guid;
        if (!AutonomousAI::CreateAutonomousCharacter(accountId, name, static_cast<uint8>(race),
            static_cast<uint8>(classId), static_cast<uint8>(gender), guid))
        {
            handler->SendSysMessage("Failed to create autonomous character. Check name, race/class and database state.");
            return false;
        }

        AutonomousAI::AutonomousBotProfile profile;
        profile.guid = guid.GetRawValue();
        profile.name = name;
        profile.enabled = true;
        if (!AutonomousAI::sAutonomousBotMgr.SaveProfile(profile))
            return false;

        AutonomousAI::sAutonomousBotMgr.StartEnabledHeadlessBots();
        handler->PSendSysMessage("Created autonomous character %s [%llu] and queued it for headless startup.",
            name.c_str(), static_cast<unsigned long long>(guid.GetRawValue()));
        return true;
    }

    static bool HandleAdd(ChatHandler* handler)
    {
        Player* player = GetTarget(handler);
        if (!player)
            return false;

        AutonomousAI::AutonomousBotProfile profile;
        profile.guid = player->GetGUID().GetRawValue();
        profile.name = player->GetName();

        if (AutonomousAI::sAutonomousBotMgr.HasProfile(profile.guid))
        {
            handler->SendSysMessage("That player is already configured as an autonomous bot.");
            return false;
        }

        if (!AutonomousAI::sAutonomousBotMgr.SaveProfile(profile))
            return false;

        if (!AutonomousAI::sAutonomousBotMgr.Attach(player, false))
            return false;

        handler->PSendSysMessage("Autonomous bot created for %s. Use .autobot start to run it.", player->GetName().c_str());
        return true;
    }

    static bool HandleDelete(ChatHandler* handler)
    {
        Player* player = GetTarget(handler);
        if (!player)
            return false;

        AutonomousAI::sAutonomousBotMgr.Detach(player->GetGUID().GetRawValue());
        AutonomousAI::sAutonomousBotMgr.RemoveProfile(player->GetGUID().GetRawValue());
        handler->PSendSysMessage("Autonomous bot profile removed for %s.", player->GetName().c_str());
        return true;
    }

    static bool HandleAttach(ChatHandler* handler)
    {
        Player* player = GetTarget(handler);
        if (!player || !AutonomousAI::sAutonomousBotMgr.HasProfile(player->GetGUID().GetRawValue()))
            return false;

        if (!AutonomousAI::sAutonomousBotMgr.Attach(player, true))
            return false;

        handler->PSendSysMessage("Autonomous bot attached to %s.", player->GetName().c_str());
        return true;
    }

    static bool HandleDetach(ChatHandler* handler)
    {
        Player* player = GetTarget(handler);
        if (!player)
            return false;
        AutonomousAI::sAutonomousBotMgr.Detach(player->GetGUID().GetRawValue());
        handler->PSendSysMessage("Autonomous bot detached from %s.", player->GetName().c_str());
        return true;
    }

    static bool HandleStart(ChatHandler* handler)
    {
        Player* player = GetTarget(handler);
        if (!player)
            return false;

        uint64 guid = player->GetGUID().GetRawValue();
        auto* controller = AutonomousAI::sAutonomousBotMgr.Find(guid);
        if (!controller)
            controller = AutonomousAI::sAutonomousBotMgr.Attach(player, false);
        if (!controller)
            return false;

        auto profile = AutonomousAI::sAutonomousBotMgr.GetProfile(guid);
        profile.guid = guid;
        profile.name = player->GetName();
        profile.enabled = true;
        if (!AutonomousAI::sAutonomousBotMgr.SaveProfile(profile))
            return false;

        controller->ConfigureExternalAI(profile.host, profile.port);
        if (!controller->StartExternalAI())
            return false;

        handler->PSendSysMessage("Autonomous AI started for %s.", player->GetName().c_str());
        return true;
    }

    static bool HandleStop(ChatHandler* handler)
    {
        Player* player = GetTarget(handler);
        if (!player)
            return false;
        uint64 guid = player->GetGUID().GetRawValue();
        auto* controller = AutonomousAI::sAutonomousBotMgr.Find(guid);
        if (!controller)
            return false;

        controller->StopExternalAI();
        auto profile = AutonomousAI::sAutonomousBotMgr.GetProfile(guid);
        profile.enabled = false;
        AutonomousAI::sAutonomousBotMgr.SaveProfile(profile);
        handler->PSendSysMessage("Autonomous AI stopped for %s.", player->GetName().c_str());
        return true;
    }

    static bool HandleInfo(ChatHandler* handler)
    {
        Player* player = GetTarget(handler);
        if (!player)
            return false;

        uint64 guid = player->GetGUID().GetRawValue();
        auto profile = AutonomousAI::sAutonomousBotMgr.GetProfile(guid);
        if (!profile.guid)
        {
            handler->SendSysMessage("Selected player is not an autonomous bot.");
            return true;
        }

        auto* controller = AutonomousAI::sAutonomousBotMgr.Find(guid);
        handler->PSendSysMessage("Bot %s: enabled=%u attached=%u external_ai=%u endpoint=%s:%u personality=%s role=%s active_quest=%u quest_state=%s",
            player->GetName().c_str(), profile.enabled ? 1 : 0, controller ? 1 : 0,
            controller && controller->IsExternalAIEnabled() ? 1 : 0, profile.host.c_str(), profile.port,
            profile.personality.c_str(), profile.role.c_str(), controller ? controller->GetActiveQuest() : 0,
            controller ? controller->GetQuestState() : "offline");
        return true;
    }

    static bool HandleQuest(ChatHandler* handler, uint32 questId)
    {
        Player* player = GetTarget(handler);
        if (!player || !questId)
            return false;
        auto* controller = AutonomousAI::sAutonomousBotMgr.Find(player->GetGUID().GetRawValue());
        if (!controller)
            return false;
        if (!controller->StartQuest(questId))
            return false;
        handler->PSendSysMessage("Bot %s assigned quest %u.", player->GetName().c_str(), questId);
        return true;
    }

    static bool HandleStartAll(ChatHandler* handler)
    {
        AutonomousAI::sAutonomousBotMgr.StartEnabledHeadlessBots();
        handler->SendSysMessage("Enabled autonomous bots are being loaded headlessly.");
        return true;
    }

    static bool HandleStopAll(ChatHandler* handler)
    {
        AutonomousAI::sAutonomousBotMgr.StopAllHeadlessBots();
        handler->SendSysMessage("All headless autonomous bots have been stopped.");
        return true;
    }

    static bool HandleEdit(ChatHandler* handler, Tail text)
    {
        Player* player = GetTarget(handler);
        if (!player || text.empty())
            return false;

        std::string raw(text);
        std::istringstream input(raw);
        std::string field;
        input >> field;
        uint64 guid = player->GetGUID().GetRawValue();
        auto profile = AutonomousAI::sAutonomousBotMgr.GetProfile(guid);
        if (!profile.guid)
            return false;

        if (field == "endpoint")
        {
            std::string host;
            uint32 port = 0;
            input >> host >> port;
            if (host.empty() || port == 0 || port > 65535)
                return false;
            profile.host = host;
            profile.port = static_cast<uint16>(port);
        }
        else if (field == "personality")
        {
            std::string value;
            input >> value;
            if (value.empty())
                return false;
            profile.personality = value;
        }
        else if (field == "role")
        {
            std::string value;
            input >> value;
            if (value != "auto" && value != "tank" && value != "healer" && value != "ranged" && value != "damage")
            {
                handler->SendSysMessage("Role must be auto, tank, healer, ranged, or damage.");
                return false;
            }
            profile.role = value;
        }
        else
        {
            handler->SendSysMessage("Usage: .autobot edit endpoint <host> <port> | personality <name> | role <auto|tank|healer|ranged|damage>");
            return false;
        }

        if (!AutonomousAI::sAutonomousBotMgr.SaveProfile(profile))
            return false;

        if (auto* controller = AutonomousAI::sAutonomousBotMgr.Find(guid))
        {
            controller->ConfigureExternalAI(profile.host, profile.port);
            controller->SetPersonality(profile.personality);
            controller->SetRole(profile.role);
        }

        handler->PSendSysMessage("Bot %s profile updated.", player->GetName().c_str());
        return true;
    }
};

void AddSC_autonomousbot_lifecycle();

void AddSC_autonomousbot_commandscript()
{
    new autonomousbot_commandscript();
    AddSC_autonomousbot_lifecycle();
}
