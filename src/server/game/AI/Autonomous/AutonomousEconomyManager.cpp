#include "AutonomousEconomyManager.h"
#include "AutonomousBotProtocol.h"
#include <algorithm>

namespace AutonomousAI
{
    void AutonomousEconomyManager::Update(uint32 diff, Perception const& perception)
    {
        if (_timer > diff)
        {
            _timer -= diff;
            return;
        }
        _timer = 5000;

        _pressure = 0;
        _reason = "none";

        if (perception.inventoryFreeSlots <= 2)
        {
            _pressure = 80;
            _reason = "inventory_full";
        }
        else if (perception.equipmentUpgradePending)
        {
            _pressure = 60;
            _reason = "equipment_upgrade";
        }
        else if (perception.inventoryFreeSlots <= 5)
        {
            _pressure = 35;
            _reason = "inventory_pressure";
        }

        if (perception.needsTown)
            _pressure = std::max<uint8>(_pressure, 90);

        _needsEconomy = _pressure >= 60;
    }
}
