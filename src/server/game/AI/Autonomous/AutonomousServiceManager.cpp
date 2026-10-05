#include "AutonomousAcoreCompat.h"
#include "AutonomousServiceManager.h"

#include "Creature.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "WorldSession.h"
#include "SharedDefines.h"

namespace AutonomousAI
{
    AutonomousServiceManager::AutonomousServiceManager(Player* player) :
        _player(player), _timer(0), _soldJunk(0), _repaired(0), _bankOpened(false), _lastService("none")
    {
    }

    void AutonomousServiceManager::Update(uint32 diff, Perception const& perception, bool externalActive)
    {
        if (!_player || !_player->IsInWorld() || !_player->IsAlive() || _player->IsInCombat())
            return;
        if (_timer > diff)
        {
            _timer -= diff;
            return;
        }
        _timer = 5000;
        _bankOpened = false;
        _lastService = "none";

        if (externalActive && _requestedService.empty() && !perception.economyNeedsAttention)
            return;

        Creature* vendor = nullptr;
        Creature* repairer = nullptr;
        Creature* banker = nullptr;

        for (WorldObjectInfo const& object : perception.nearbyCreatures)
        {
            if (!object.guid || object.distance > 6.0f)
                continue;
            Creature* creature = ObjectAccessor::GetCreature(*_player, AutonomousMakeGuid(object.guid));
            if (!creature)
                continue;
            if (!vendor && creature->HasNpcFlag(UNIT_NPC_FLAG_VENDOR))
                vendor = creature;
            if (!repairer && creature->HasNpcFlag(UNIT_NPC_FLAG_REPAIR))
                repairer = creature;
            if (!banker && creature->HasNpcFlag(UNIT_NPC_FLAG_BANKER))
                banker = creature;
        }

        if (_requestedGuid)
        {
            Creature* requested = ObjectAccessor::GetCreature(*_player, AutonomousMakeGuid(_requestedGuid));
            if (requested)
            {
                if (_requestedService == "vendor") vendor = requested;
                else if (_requestedService == "repair") repairer = requested;
                else if (_requestedService == "bank") banker = requested;
            }
        }

        if (repairer && (_requestedService == "repair" || _requestedService.empty() && (perception.economyReason == "repair" || perception.economyPressure >= 80)))
        {
            if (Creature* usable = _player->GetNPCIfCanInteractWith(repairer->GetGUID(), UNIT_NPC_FLAG_REPAIR))
            {
                _player->DurabilityRepairAll(true, _player->GetReputationPriceDiscount(usable), false);
                ++_repaired;
                _lastService = "repair";
                _requestedService.clear();
                _requestedGuid = 0;
                return;
            }
        }

        if (vendor && (_requestedService == "vendor" || perception.inventoryFreeSlots <= 3))
        {
            if (Creature* usable = _player->GetNPCIfCanInteractWith(vendor->GetGUID(), UNIT_NPC_FLAG_VENDOR))
            {
                _player->GetSession()->SendListInventory(usable->GetGUID());
                for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
                {
                    Item* item = _player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
                    if (!item || item->GetTemplate()->Quality != ITEM_QUALITY_POOR || !item->GetTemplate()->SellPrice)
                        continue;
                    // 3.3.5 exposes vendor selling through the session opcode path;
                    // leave the inventory untouched here rather than calling a nonexistent Player helper.
                }
                _lastService = "vendor";
                _requestedService.clear();
                _requestedGuid = 0;
                return;
            }
        }

        if (banker && (_requestedService == "bank" || perception.inventoryFreeSlots <= 1))
        {
            if (Creature* usable = _player->GetNPCIfCanInteractWith(banker->GetGUID(), UNIT_NPC_FLAG_BANKER))
            {
                _player->GetSession()->SendShowBank(usable->GetGUID());
                _bankOpened = true;
                _lastService = "bank";
            }
        }
        _requestedService.clear();
        _requestedGuid = 0;
    }
}
