#include "AutonomousAcoreCompat.h"
#include "AutonomousHeadlessPlayer.h"

#include "CharacterCache.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "MotionMaster.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "World.h"
#include "WorldSession.h"

namespace AutonomousAI
{
    bool CreateAutonomousCharacter(uint32 accountId, std::string const& name, uint8 race, uint8 classId, uint8 gender, ObjectGuid& outGuid)
    {
        if (!accountId || name.empty() || !sObjectMgr->GetPlayerInfo(race, classId))
            return false;

        if (ObjectMgr::CheckPlayerName(name, true) != CHAR_NAME_SUCCESS)
            return false;

        if (sCharacterCache->GetCharacterCacheByName(name))
            return false;

        CharacterDatabasePreparedStatement* checkName = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHECK_NAME);
        checkName->SetData(0, name);
        if (CharacterDatabase.Query(checkName))
            return false;

        auto session = std::make_unique<WorldSession>(accountId, std::string("AutonomousAI"), 0, nullptr,
            SEC_PLAYER, 2, 0, LOCALE_enUS, 0, false, true, 0);

        CharacterCreateInfo createInfo(name, race, classId, gender, 0, 0, 0, 0, 0);

        auto player = std::make_unique<Player>(session.get());
        player->GetMotionMaster()->Initialize();

        ObjectGuid guid = ObjectGuid::Create<HighGuid::Player>(sObjectMgr->GetGenerator<HighGuid::Player>().Generate());
        if (!player->Create(guid.GetCounter(), &createInfo))
            return false;

        player->SetAtLoginFlag(AT_LOGIN_FIRST);

        CharacterDatabaseTransaction transaction = CharacterDatabase.BeginTransaction();
        player->SaveToDB(transaction, true, false);
        CharacterDatabase.CommitTransaction(transaction);

        outGuid = player->GetGUID();
        sCharacterCache->AddCharacterCacheEntry(outGuid, accountId, player->GetName(), player->getGender(),
            player->getRace(), player->getClass(), player->GetLevel());

        LOG_INFO("entities.player.character", "Autonomous AI created character: {} {} (account {})",
            player->GetName(), outGuid.ToString(), accountId);

        player->CleanupsBeforeDelete();
        player.reset();
        return true;
    }

    void LoadHeadlessPlayer(ObjectGuid guid, HeadlessPlayerCallback callback)
    {
        CharacterCacheEntry const* characterInfo = sCharacterCache->GetCharacterCacheByGuid(guid);
        if (!characterInfo)
        {
            if (callback)
                callback(nullptr, nullptr);
            return;
        }

        auto* session = new WorldSession(characterInfo->AccountId, std::string("AutonomousAI"), 0, nullptr,
            SEC_PLAYER, 2, 0, LOCALE_enUS, 0, false, true, 0);

        auto holder = std::make_shared<LoginQueryHolder>(characterInfo->AccountId, guid);
        if (!holder->Initialize())
        {
            delete session;
            if (callback)
                callback(nullptr, nullptr);
            return;
        }

        session->AddQueryHolderCallback(CharacterDatabase.DelayQueryHolder(holder))
            .AfterComplete([session, callback](SQLQueryHolderBase const& queryHolder)
        {
            session->HandlePlayerLoginFromDB(static_cast<LoginQueryHolder const&>(queryHolder));

            if (callback)
            {
                if (Player* player = session->GetPlayer())
                    callback(player, session);
                else
                {
                    callback(nullptr, nullptr);
                    delete session;
                }
            }
            else if (!session->GetPlayer())
                delete session;
        });
    }

    void UnloadHeadlessPlayer(WorldSession* session)
    {
        if (!session)
            return;

        session->LogoutPlayer(true);
        delete session;
    }
}
