#ifndef TRINITY_AUTONOMOUS_EQUIPMENT_MANAGER_H
#define TRINITY_AUTONOMOUS_EQUIPMENT_MANAGER_H

#include "Common.h"
#include <string>

class Player;

namespace AutonomousAI
{
    class AutonomousEquipmentManager
    {
    public:
        explicit AutonomousEquipmentManager(Player* player);
        void Update(uint32 diff, bool externalActive);
        uint32 GetUpgradesEquipped() const { return _upgradesEquipped; }
        uint32 GetInventorySpace() const { return _inventorySpace; }
        bool HasUpgradePending() const { return _upgradePending; }

    private:
        Player* _player;
        uint32 _timer;
        uint32 _upgradesEquipped;
        uint32 _inventorySpace;
        bool _upgradePending;
    };
}
#endif
