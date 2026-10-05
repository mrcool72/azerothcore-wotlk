#include "AutonomousBotMgr.h"
#include "Config.h"
#include "AutonomousBotController.h"
#include "AutonomousBotProfileStore.h"
#include "AutonomousHeadlessPlayer.h"
#include "AutonomousPlayerAI.h"
#include "AutonomousSocialManager.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "WorldSession.h"

namespace AutonomousAI
{
    AutonomousBotMgr sAutonomousBotMgr;

    AutonomousBotMgr::~AutonomousBotMgr()
    {
        if (_social)
            _social->Shutdown();
        StopAllHeadlessBots();
    }

    void AutonomousBotMgr::LoadProfiles()
    {
        sAutonomousBotProfileStore.Load();
        if (!_social)
            _social = std::make_unique<AutonomousSocialManager>();
        StartEnabledHeadlessBots();
    }

    AutonomousBotController* AutonomousBotMgr::Attach(Player* player, bool startIfEnabled)
    {
        if (!player)
            return nullptr;

        uint64 guid = player->GetGUID().GetRawValue();
        auto [itr, inserted] = _controllers.emplace(guid, nullptr);
        if (inserted)
        {
            auto controller = std::make_unique<AutonomousBotController>(player);
            AutonomousBotController* result = controller.get();
            auto* ai = new AutonomousPlayerAI(player);
            ai->SetController(result);
            result->SetAI(ai);
            player->SetAI(ai);
            itr->second = std::move(controller);
        }

        AutonomousBotProfile profile;
        if (sAutonomousBotProfileStore.Get(guid, profile))
        {
            bool const externalEnabled = sConfigMgr->GetOption<bool>("AutonomousAI.Enable", false, false);
            bool const overrideProfileEndpoint = sConfigMgr->GetOption<bool>("AutonomousAI.OverrideProfileEndpoint", true, false);
            std::string const globalHost = sConfigMgr->GetOption<std::string>("AutonomousAI.Host", "127.0.0.1", false);
            uint16 const globalPort = sConfigMgr->GetOption<uint16>("AutonomousAI.Port", 8765, false);
            std::string const token = sConfigMgr->GetOption<std::string>("AutonomousAI.Token", "", false);
            uint32 const requestInterval = sConfigMgr->GetOption<uint32>("AutonomousAI.RequestInterval", 1500, false);

            std::string host = overrideProfileEndpoint ? globalHost : profile.host;
            uint16 port = overrideProfileEndpoint ? globalPort : profile.port;

            itr->second->ConfigureExternalAI(std::move(host), port, token);
            itr->second->SetExternalAIRequestInterval(requestInterval);
            itr->second->SetPersonality(profile.personality);
            itr->second->SetRole(profile.role);

            if (startIfEnabled && profile.enabled && externalEnabled)
                itr->second->StartExternalAI();
        }

        return itr->second.get();
    }

    void AutonomousBotMgr::Detach(uint64 guid)
    {
        _controllers.erase(guid);

        if (IsHeadless(guid))
            StopHeadless(guid);
    }

    bool AutonomousBotMgr::ShouldDeferDungeonBrain(Player* player) const
    {
        return _social && _social->ShouldDeferDungeonBrain(player);
    }

    void AutonomousBotMgr::Update(uint32 diff)
    {
        // Controllers are driven by their UnitAI through Unit::AIUpdateTick().
        // Keeping the controller loop here as well would execute every autonomous
        // controller twice per world tick. The manager retains the global social
        // update, while each bot's AI owns its controller cadence.
        if (_social)
            _social->Update(diff);
    }

    void AutonomousBotMgr::OnLogin(Player* player)
    {
        if (!player || !HasProfile(player->GetGUID().GetRawValue()))
            return;

        // A configured headless bot already owns this character. A dedicated bot account
        // should never be used for an interactive client at the same time.
        if (IsHeadless(player->GetGUID().GetRawValue()))
            return;

        Attach(player, true);
    }

    void AutonomousBotMgr::OnLogout(Player* player)
    {
        if (player)
            _controllers.erase(player->GetGUID().GetRawValue());
    }

    void AutonomousBotMgr::StartEnabledHeadlessBots()
    {
        for (AutonomousBotProfile const& profile : sAutonomousBotProfileStore.GetAll())
        {
            if (!profile.enabled)
                continue;

            uint64 rawGuid = profile.guid;
            ObjectGuid guid = AutonomousMakeGuid(rawGuid);

            if (ObjectAccessor::FindPlayer(guid))
            {
                Attach(ObjectAccessor::FindPlayer(guid), true);
                continue;
            }

            if (_headless.contains(rawGuid) || _pendingHeadlessLoads.contains(rawGuid))
                continue;

            _pendingHeadlessLoads.insert(rawGuid);
            LoadHeadlessPlayer(guid, [this, rawGuid](Player* player, WorldSession* session)
            {
                _pendingHeadlessLoads.erase(rawGuid);

                if (!player || !session)
                    return;

                if (_headless.contains(rawGuid))
                {
                    session->LogoutPlayer(true);
                    delete session;
                    return;
                }

                _headless.emplace(rawGuid, HeadlessRuntime{ player, session });

                if (!Attach(player, true))
                {
                    _headless.erase(rawGuid);
                    session->LogoutPlayer(true);
                    delete session;
                    return;
                }
            });
        }
    }

    void AutonomousBotMgr::StopHeadless(uint64 guid)
    {
        auto itr = _headless.find(guid);
        if (itr == _headless.end())
            return;

        _controllers.erase(guid);

        WorldSession* session = itr->second.session;
        _headless.erase(itr);
        UnloadHeadlessPlayer(session);
    }

    void AutonomousBotMgr::StopAllHeadlessBots()
    {
        // Detach only controllers owned by headless players. Interactive players may still
        // be online when this hook runs.
        for (auto const& [guid, runtime] : _headless)
            _controllers.erase(guid);

        std::vector<WorldSession*> sessions;
        sessions.reserve(_headless.size());
        for (auto const& [guid, runtime] : _headless)
            if (runtime.session)
                sessions.push_back(runtime.session);

        _headless.clear();
        _pendingHeadlessLoads.clear();

        for (WorldSession* session : sessions)
            UnloadHeadlessPlayer(session);
    }

    bool AutonomousBotMgr::IsHeadless(uint64 guid) const
    {
        return _headless.find(guid) != _headless.end();
    }

    AutonomousBotController* AutonomousBotMgr::Find(uint64 guid) const
    {
        auto itr = _controllers.find(guid);
        return itr != _controllers.end() ? itr->second.get() : nullptr;
    }

    AutonomousBotProfile AutonomousBotMgr::GetProfile(uint64 guid) const
    {
        AutonomousBotProfile profile;
        sAutonomousBotProfileStore.Get(guid, profile);
        return profile;
    }

    bool AutonomousBotMgr::HasProfile(uint64 guid) const
    {
        AutonomousBotProfile profile;
        return sAutonomousBotProfileStore.Get(guid, profile);
    }

    bool AutonomousBotMgr::SaveProfile(AutonomousBotProfile const& profile)
    {
        return sAutonomousBotProfileStore.Save(profile);
    }

    bool AutonomousBotMgr::RemoveProfile(uint64 guid)
    {
        StopHeadless(guid);
        return sAutonomousBotProfileStore.Remove(guid);
    }

    std::vector<AutonomousBotProfile> AutonomousBotMgr::GetProfiles() const
    {
        return sAutonomousBotProfileStore.GetAll();
    }
}
