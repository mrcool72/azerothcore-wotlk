#ifndef TRINITY_AUTONOMOUS_COMBAT_MANAGER_H
#define TRINITY_AUTONOMOUS_COMBAT_MANAGER_H

#include "Common.h"

#include <cstddef>
#include <string>

class Player;
class Unit;

namespace AutonomousAI
{
    enum class AutonomousCombatRole : uint8
    {
        DAMAGE = 0,
        TANK,
        HEALER,
        RANGED
    };

    class AutonomousCombatManager
    {
    public:
        explicit AutonomousCombatManager(Player* player);

        void Update(uint32 diff);
        void SetConfiguredRole(std::string role);
        AutonomousCombatRole GetRole() const { return _role; }
        char const* GetRoleName() const;

    private:
        void DetermineRole();
        bool HealGroup();
        bool TankAssist();
        bool AssistGroup();
        bool PositionForEncounter();
        bool InterruptBossCast();
        bool FindGroupTarget(Player*& member, Unit*& victim) const;
        bool CastKnownSpell(Player* target, uint32 const* spellIds, size_t count) const;
        bool CastKnownSpell(Unit* target, uint32 const* spellIds, size_t count) const;

        Player* _player;
        AutonomousCombatRole _role;
        std::string _configuredRole;
        uint32 _timer;
        uint32 _healTimer;
        uint32 _tauntTimer;
    };
}

#endif
