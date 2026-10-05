#include "AutonomousAcoreCompat.h"
#include "AutonomousDungeonManager.h"
#include "AutonomousBotProtocol.h"

#include "Creature.h"
#include "Group.h"
#include "LootMgr.h"
#include "Map.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "ObjectGuid.h"
#include "Player.h"

namespace AutonomousAI
{
    namespace
    {
        constexpr float LOOT_RANGE = 6.0f;
        constexpr uint32 LOOT_RETRY_INTERVAL = 1000;
    }

    AutonomousDungeonManager::AutonomousDungeonManager(Player* player) :
        _player(player),
        _lastBossGuid(0),
        _encounterActive(false),
        _lootTimer(0)
    {
    }

    bool AutonomousDungeonManager::IsAutonomousLeader() const
    {
        if (!_player || !_player->IsInWorld() || !_player->GetMap() || !_player->GetMap()->IsDungeon())
            return false;

        Group* group = _player->GetGroup();
        return group && group->GetLeaderGUID() == _player->GetGUID();
    }

    bool AutonomousDungeonManager::MoveToBossCorpse()
    {
        if (!_player || !_lastBossGuid)
            return false;

        Creature* boss = ObjectAccessor::GetCreature(*_player, AutonomousMakeGuid(_lastBossGuid));
        if (!boss || boss->IsAlive() || !boss->loot.unlootedCount && boss->loot.gold == 0)
            return false;

        if (_player->GetDistance(boss) > LOOT_RANGE)
        {
            _player->GetMotionMaster()->MovePoint(0, boss->GetPositionX(), boss->GetPositionY(), boss->GetPositionZ(), FORCED_MOVEMENT_NONE);
            return true;
        }

        return false;
    }

    bool AutonomousDungeonManager::LootBossCorpse()
    {
        if (!_player || !_lastBossGuid)
            return false;

        Creature* boss = ObjectAccessor::GetCreature(*_player, AutonomousMakeGuid(_lastBossGuid));
        if (!boss || boss->IsAlive())
            return false;

        if (_player->GetDistance(boss) > LOOT_RANGE)
            return MoveToBossCorpse();

        if (boss->loot.empty())
        {
            _lastBossGuid = 0;
            _encounterActive = false;
            return false;
        }

        _player->SetLootGUID(boss->GetGUID());

        uint32 maxSlot = boss->loot.GetMaxSlotInLootFor(_player);
        for (uint32 slot = 0; slot < maxSlot; ++slot)
        {
            InventoryResult lootResult;
            _player->StoreLootItem(static_cast<uint8>(slot), &boss->loot, lootResult);
        }

        if (boss->loot.gold)
        {
            uint32 gold = boss->loot.gold;
            boss->loot.gold = 0;
            _player->ModifyMoney(static_cast<int32>(gold));
            boss->loot.NotifyMoneyRemoved();
        }

        if (boss->loot.isLooted())
        {
            boss->AllLootRemovedFromCorpse();
            _player->SetLootGUID(ObjectGuid::Empty);
            _lastBossGuid = 0;
            _encounterActive = false;
        }

        return true;
    }


    bool AutonomousDungeonManager::NavigateToNextBoss(Perception const& perception, bool externalAIActive)
    {
        if (!_player || !IsAutonomousLeader() || externalAIActive || perception.groupInCombat ||
            perception.dungeonRegroupRequired || !perception.dungeonNextBossGuid)
            return false;

        Creature* boss = ObjectAccessor::GetCreature(*_player, AutonomousMakeGuid(perception.dungeonNextBossGuid));
        if (!boss || !boss->IsAlive())
            return false;

        if (_player->GetDistance(boss) <= 8.0f)
            return false;

        _player->GetMotionMaster()->MovePoint(0, boss->GetPositionX(), boss->GetPositionY(), boss->GetPositionZ(), FORCED_MOVEMENT_NONE);
        return true;
    }

    void AutonomousDungeonManager::Update(uint32 diff, Perception const& perception, bool externalAIActive)
    {
        if (!_player || !_player->IsInWorld() || !perception.inDungeon)
            return;

        if (perception.dungeonPullTargetIsBoss && perception.dungeonPullTargetGuid)
        {
            _lastBossGuid = perception.dungeonPullTargetGuid;
            _encounterActive = true;
        }

        if (!_encounterActive && perception.bossGuid && perception.bossDead)
            _lastBossGuid = perception.bossGuid;

        if (!_encounterActive)
            NavigateToNextBoss(perception, externalAIActive);

        if (!_encounterActive && _lastBossGuid)
        {
            if (_lootTimer > diff)
                _lootTimer -= diff;
            else
            {
                _lootTimer = LOOT_RETRY_INTERVAL;
                if (IsAutonomousLeader())
                    LootBossCorpse();
            }
        }

        if (perception.dungeonState == "boss_dead" && IsAutonomousLeader())
        {
            if (_lootTimer > diff)
                _lootTimer -= diff;
            else
            {
                _lootTimer = LOOT_RETRY_INTERVAL;
                LootBossCorpse();
            }
        }
    }
}

