#ifndef TRINITY_AUTONOMOUS_DUNGEON_MANAGER_H
#define TRINITY_AUTONOMOUS_DUNGEON_MANAGER_H

#include "AutonomousBotProtocol.h"

#include <cstdint>

class Player;

namespace AutonomousAI
{
    class AutonomousDungeonManager
    {
    public:
        explicit AutonomousDungeonManager(Player* player);

        void Update(uint32 diff, Perception const& perception, bool externalAIActive);
        uint64 GetLastBossGuid() const { return _lastBossGuid; }
        bool HasActiveEncounter() const { return _encounterActive; }

    private:
        bool IsAutonomousLeader() const;
        bool LootBossCorpse();
        bool MoveToBossCorpse();
        bool NavigateToNextBoss(Perception const& perception, bool externalAIActive);

        Player* _player;
        uint64 _lastBossGuid;
        bool _encounterActive;
        uint32 _lootTimer;
};
}

#endif
