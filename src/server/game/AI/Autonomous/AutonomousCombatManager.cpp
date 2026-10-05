#include "AutonomousCombatManager.h"

#include "Group.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Map.h"
#include "MotionMaster.h"
#include "PetDefines.h"
#include "Spell.h"
#include "Unit.h"

#include <cstddef>
#include <utility>

namespace AutonomousAI
{
    namespace
    {
        constexpr uint32 THINK_INTERVAL = 500;
        constexpr uint32 HEAL_INTERVAL = 1500;
        constexpr uint32 TAUNT_INTERVAL = 1800;
        constexpr uint32 POSITION_INTERVAL = 1000;

        // WotLK spell IDs, ordered from the commonly available lower-rank spell
        // to stronger spells. HasSpell() makes this safe across levels/builds.
        constexpr uint32 PRIEST_HEALS[] = { 48071, 48068, 48066, 2061, 2050 };
        constexpr uint32 SHAMAN_HEALS[] = { 49276, 49273, 49269, 49263, 8004 };
        constexpr uint32 DRUID_HEALS[] = { 48378, 48377, 48375, 26979, 5185 };
        constexpr uint32 PALADIN_HEALS[] = { 48785, 48782, 48781, 27135, 635 };

        constexpr uint32 WARRIOR_TAUNTS[] = { 355, 1161 };
        constexpr uint32 PALADIN_TAUNTS[] = { 31789, 62124 };
        constexpr uint32 DK_TAUNTS[] = { 56222, 49576 };
        constexpr uint32 DRUID_TAUNTS[] = { 6795 };

        bool IsHealerClass(uint8 classId)
        {
            return classId == CLASS_PRIEST || classId == CLASS_SHAMAN ||
                classId == CLASS_PALADIN || classId == CLASS_DRUID;
        }

        bool IsRangedClass(uint8 classId)
        {
            return classId == CLASS_HUNTER || classId == CLASS_MAGE ||
                classId == CLASS_WARLOCK || classId == CLASS_PRIEST ||
                classId == CLASS_SHAMAN;
        }

        bool IsTankClass(uint8 classId)
        {
            return classId == CLASS_WARRIOR || classId == CLASS_PALADIN ||
                classId == CLASS_DEATH_KNIGHT || classId == CLASS_DRUID;
        }
    }

    AutonomousCombatManager::AutonomousCombatManager(Player* player) :
        _player(player),
        _role(AutonomousCombatRole::DAMAGE),
        _configuredRole("auto"),
        _timer(0),
        _healTimer(0),
        _tauntTimer(0)
    {
        DetermineRole();
    }

    void AutonomousCombatManager::SetConfiguredRole(std::string role)
    {
        if (role == "auto" || role == "tank" || role == "healer" || role == "ranged" || role == "damage")
            _configuredRole = std::move(role);
        else
            _configuredRole = "auto";

        DetermineRole();
    }

    void AutonomousCombatManager::DetermineRole()
    {
        if (!_player)
            return;

        if (_configuredRole == "tank")
        {
            _role = AutonomousCombatRole::TANK;
            return;
        }
        if (_configuredRole == "healer")
        {
            _role = AutonomousCombatRole::HEALER;
            return;
        }
        if (_configuredRole == "ranged")
        {
            _role = AutonomousCombatRole::RANGED;
            return;
        }
        if (_configuredRole == "damage")
        {
            _role = AutonomousCombatRole::DAMAGE;
            return;
        }

        uint8 classId = _player->getClass();
        if (IsHealerClass(classId))
            _role = AutonomousCombatRole::HEALER;
        else if (IsTankClass(classId))
            _role = AutonomousCombatRole::TANK;
        else if (IsRangedClass(classId))
            _role = AutonomousCombatRole::RANGED;
        else
            _role = AutonomousCombatRole::DAMAGE;

        if (_player->GetLevel() < 10 && _role == AutonomousCombatRole::HEALER)
            _role = AutonomousCombatRole::DAMAGE;
    }

    char const* AutonomousCombatManager::GetRoleName() const
    {
        switch (_role)
        {
            case AutonomousCombatRole::TANK:   return "tank";
            case AutonomousCombatRole::HEALER: return "healer";
            case AutonomousCombatRole::RANGED: return "ranged";
            default:                            return "damage";
        }
    }

    bool AutonomousCombatManager::CastKnownSpell(Player* target, uint32 const* spellIds, size_t count) const
    {
        return CastKnownSpell(static_cast<Unit*>(target), spellIds, count);
    }

    bool AutonomousCombatManager::CastKnownSpell(Unit* target, uint32 const* spellIds, size_t count) const
    {
        if (!_player || !target || !target->IsAlive())
            return false;

        for (size_t i = 0; i < count; ++i)
        {
            if (!spellIds[i] || !_player->HasSpell(spellIds[i]))
                continue;

            _player->CastSpell(target, spellIds[i], false);
            return true;
        }

        return false;
    }

    bool AutonomousCombatManager::HealGroup()
    {
        if (!_player || !IsHealerClass(_player->getClass()))
            return false;

        Group* group = _player->GetGroup();
        if (!group)
            return false;

        Player* best = nullptr;
        uint32 lowestPct = 101;

        for (Group::member_citerator itr = group->GetMemberSlots().begin(); itr != group->GetMemberSlots().end(); ++itr)
        {
            Player* member = ObjectAccessor::FindPlayer(itr->guid);
            if (!member || !member->IsInWorld() || !member->IsAlive() || member->GetMapId() != _player->GetMapId())
                continue;

            if (_player->GetDistance(member) > 40.0f)
                continue;

            uint32 pct = member->GetHealthPct();
            if (pct < 70 && pct < lowestPct)
            {
                lowestPct = pct;
                best = member;
            }
        }

        if (!best)
            return false;

        switch (_player->getClass())
        {
            case CLASS_PRIEST:
                return CastKnownSpell(best, PRIEST_HEALS, sizeof(PRIEST_HEALS) / sizeof(PRIEST_HEALS[0]));
            case CLASS_SHAMAN:
                return CastKnownSpell(best, SHAMAN_HEALS, sizeof(SHAMAN_HEALS) / sizeof(SHAMAN_HEALS[0]));
            case CLASS_DRUID:
                return CastKnownSpell(best, DRUID_HEALS, sizeof(DRUID_HEALS) / sizeof(DRUID_HEALS[0]));
            case CLASS_PALADIN:
                return CastKnownSpell(best, PALADIN_HEALS, sizeof(PALADIN_HEALS) / sizeof(PALADIN_HEALS[0]));
            default:
                return false;
        }
    }

    bool AutonomousCombatManager::FindGroupTarget(Player*& member, Unit*& victim) const
    {
        member = nullptr;
        victim = nullptr;
        if (!_player)
            return false;

        Group* group = _player->GetGroup();
        if (!group)
            return false;

        if (_player->GetMap() && _player->GetMap()->IsDungeon() && ObjectAccessor::FindPlayer(group->GetLeaderGUID()))
        {
            Player* leader = ObjectAccessor::FindPlayer(group->GetLeaderGUID());
            Unit* target = leader ? leader->GetVictim() : nullptr;
            if (target && target->IsAlive() && _player->IsValidAttackTarget(target))
            {
                member = leader;
                victim = target;
                return true;
            }
            return false;
        }

        for (Group::member_citerator itr = group->GetMemberSlots().begin(); itr != group->GetMemberSlots().end(); ++itr)
        {
            Player* candidate = ObjectAccessor::FindPlayer(itr->guid);
            if (!candidate || !candidate->IsAlive() || candidate->GetMapId() != _player->GetMapId())
                continue;

            Unit* target = candidate->GetVictim();
            if (!target || !target->IsAlive() || !_player->IsValidAttackTarget(target))
                continue;

            member = candidate;
            victim = target;
            return true;
        }

        return false;
    }

    bool AutonomousCombatManager::TankAssist()
    {
        if (!_player || _role != AutonomousCombatRole::TANK)
            return false;

        Player* member = nullptr;
        Unit* victim = nullptr;
        if (!FindGroupTarget(member, victim))
            return false;

        if (_tauntTimer != 0)
            return false;

        bool taunted = false;
        switch (_player->getClass())
        {
            case CLASS_WARRIOR:
                taunted = CastKnownSpell(victim, WARRIOR_TAUNTS, sizeof(WARRIOR_TAUNTS) / sizeof(WARRIOR_TAUNTS[0]));
                break;
            case CLASS_PALADIN:
                taunted = CastKnownSpell(victim, PALADIN_TAUNTS, sizeof(PALADIN_TAUNTS) / sizeof(PALADIN_TAUNTS[0]));
                break;
            case CLASS_DEATH_KNIGHT:
                taunted = CastKnownSpell(victim, DK_TAUNTS, sizeof(DK_TAUNTS) / sizeof(DK_TAUNTS[0]));
                break;
            case CLASS_DRUID:
                taunted = CastKnownSpell(victim, DRUID_TAUNTS, sizeof(DRUID_TAUNTS) / sizeof(DRUID_TAUNTS[0]));
                break;
            default:
                break;
        }

        if (taunted)
            _tauntTimer = TAUNT_INTERVAL;

        if (_player->IsValidAttackTarget(victim))
            _player->Attack(victim, true);

        (void)member;
        return true;
    }

    bool AutonomousCombatManager::AssistGroup()
    {
        Player* member = nullptr;
        Unit* victim = nullptr;
        if (!FindGroupTarget(member, victim))
            return false;

        if (_role == AutonomousCombatRole::HEALER)
            return false;

        if (_player->IsValidAttackTarget(victim))
        {
            _player->Attack(victim, true);
            return true;
        }

        return false;
    }

    bool AutonomousCombatManager::PositionForEncounter()
    {
        if (!_player || !_player->GetMap() || !_player->GetMap()->IsDungeon())
            return false;

        Group* group = _player->GetGroup();
        if (!group)
            return false;

        Player* leader = ObjectAccessor::FindPlayer(group->GetLeaderGUID());
        Unit* boss = leader ? leader->GetVictim() : nullptr;
        if (!boss || !boss->IsAlive() || !boss->ToCreature() || boss->ToCreature()->GetCreatureTemplate()->rank < 3)
            return false;

        float distance = _player->GetDistance(boss);
        if (_role == AutonomousCombatRole::TANK)
        {
            if (distance > 8.0f)
                _player->GetMotionMaster()->MoveChase(boss, 4.0f);
            else if (distance < 2.0f)
                _player->GetMotionMaster()->MoveChase(boss, 3.0f);
            return true;
        }

        if (_role == AutonomousCombatRole::HEALER || _role == AutonomousCombatRole::RANGED)
        {
            if (distance < 12.0f || distance > 35.0f)
                _player->GetMotionMaster()->MoveFollow(leader, _role == AutonomousCombatRole::HEALER ? 14.0f : 20.0f, PET_FOLLOW_ANGLE);
            return true;
        }

        if (distance > 12.0f)
            _player->GetMotionMaster()->MoveChase(boss, 4.0f);
        return true;
    }

    bool AutonomousCombatManager::InterruptBossCast()
    {
        if (!_player || !_player->GetMap() || !_player->GetMap()->IsDungeon())
            return false;

        Group* group = _player->GetGroup();
        Player* leader = group ? ObjectAccessor::FindPlayer(group->GetLeaderGUID()) : nullptr;
        Unit* boss = leader ? leader->GetVictim() : nullptr;
        if (!boss || !boss->IsAlive() || !boss->ToCreature() ||
            !boss->ToCreature()->GetCreatureTemplate() || boss->ToCreature()->GetCreatureTemplate()->rank < 3 ||
            !boss->HasUnitState(UNIT_STATE_CASTING))
            return false;

        static uint32 const INTERRUPTS[] = {
            6552,   // Warrior: Pummel
            1766,   // Rogue: Kick
            2139,   // Mage: Counterspell
            57994,  // Shaman: Wind Shear
            15487,  // Priest: Silence
            47528,  // Death Knight: Mind Freeze
            34490,  // Hunter: Silencing Shot
            19647   // Warlock: Spell Lock
        };

        if (_player->GetDistance(boss) > 30.0f)
            return false;

        for (uint32 spellId : INTERRUPTS)
        {
            if (!_player->HasSpell(spellId))
                continue;

            _player->CastSpell(boss, spellId, false);
            return true;
        }

        return false;
    }

    void AutonomousCombatManager::Update(uint32 diff)
    {
        if (!_player || !_player->IsInWorld() || !_player->IsAlive())
            return;

        DetermineRole();

        if (_timer > diff)
            _timer -= diff;
        else
        {
            _timer = THINK_INTERVAL;

            if (_role == AutonomousCombatRole::HEALER && HealGroup())
                return;

            if (InterruptBossCast())
                return;

            if (PositionForEncounter())
                return;

            if (_role == AutonomousCombatRole::TANK && TankAssist())
                return;

            AssistGroup();
        }

        if (_healTimer > diff)
            _healTimer -= diff;
        else
            _healTimer = HEAL_INTERVAL;

        if (_tauntTimer > diff)
            _tauntTimer -= diff;
        else
            _tauntTimer = 0;
    }
}

