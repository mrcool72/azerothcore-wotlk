#include "AutonomousAcoreCompat.h"
#include "AutonomousEquipmentManager.h"
#include "Item.h"
#include "Player.h"
#include "SharedDefines.h"

namespace AutonomousAI
{
    AutonomousEquipmentManager::AutonomousEquipmentManager(Player* player) :
        _player(player), _timer(0), _upgradesEquipped(0), _inventorySpace(0), _upgradePending(false)
    {
    }

    void AutonomousEquipmentManager::Update(uint32 diff, bool externalActive)
    {
        if (!_player || !_player->IsInWorld())
            return;
        _inventorySpace = _player->GetFreeInventorySpace();
        if (externalActive)
            return;
        if (_timer > diff) { _timer -= diff; return; }
        _timer = 5000;
        _upgradePending = false;

        // Only inspect the player's main inventory. FindEquipSlot/CanEquipItem
        // remain the authoritative class/race/skill checks.
        for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
        {
            Item* item = _player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
            if (!item || item->IsEquipped())
                continue;

            uint16 destination = _player->FindEquipSlot(item->GetTemplate(), NULL_SLOT, false);
            if (destination == NULL_SLOT)
                continue;

            Item* equipped = _player->GetItemByPos(destination);
            if (equipped && equipped->GetTemplate()->ItemLevel >= item->GetTemplate()->ItemLevel)
                continue;

            if (_player->CanEquipItem(0, destination, item, true) == EQUIP_ERR_OK)
            {
                if (_player->EquipItem(destination, item, true))
                    ++_upgradesEquipped;
                else
                    _upgradePending = true;
            }
        }
    }
}
