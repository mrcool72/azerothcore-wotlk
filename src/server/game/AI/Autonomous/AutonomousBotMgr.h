#ifndef TRINITY_AUTONOMOUS_BOT_MGR_H
#define TRINITY_AUTONOMOUS_BOT_MGR_H

#include "Common.h"
#include "AutonomousBotProfile.h"
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Player;
class WorldSession;

namespace AutonomousAI
{
    class AutonomousBotController;
    class AutonomousSocialManager;

    class AutonomousBotMgr
    {
    public:
        AutonomousBotMgr() = default;
        ~AutonomousBotMgr();
        AutonomousBotMgr(AutonomousBotMgr const&) = delete;
        AutonomousBotMgr& operator=(AutonomousBotMgr const&) = delete;

        void LoadProfiles();
        AutonomousBotController* Attach(Player* player, bool startIfEnabled = true);
        void Detach(uint64 guid);
        void Update(uint32 diff);
        void OnLogin(Player* player);
        void OnLogout(Player* player);

        // Phase 6: load enabled characters through TrinityCore's normal login pipeline,
        // but with a null WorldSocket. The character then exists in the world without a client.
        void StartEnabledHeadlessBots();
        void StopHeadless(uint64 guid);
        void StopAllHeadlessBots();
        bool IsHeadless(uint64 guid) const;

        AutonomousBotController* Find(uint64 guid) const;
        AutonomousBotProfile GetProfile(uint64 guid) const;
        bool HasProfile(uint64 guid) const;
        bool SaveProfile(AutonomousBotProfile const& profile);
        bool RemoveProfile(uint64 guid);
        std::vector<AutonomousBotProfile> GetProfiles() const;
        bool ShouldDeferDungeonBrain(Player* player) const;
        size_t Size() const { return _controllers.size(); }
        size_t HeadlessSize() const { return _headless.size(); }

    private:
        struct HeadlessRuntime
        {
            Player* player = nullptr;
            WorldSession* session = nullptr;
        };

        std::unordered_map<uint64, std::unique_ptr<AutonomousBotController>> _controllers;
        std::unordered_map<uint64, HeadlessRuntime> _headless;
        std::unordered_set<uint64> _pendingHeadlessLoads;
        std::unique_ptr<AutonomousSocialManager> _social;
    };

    extern AutonomousBotMgr sAutonomousBotMgr;
}

#endif
