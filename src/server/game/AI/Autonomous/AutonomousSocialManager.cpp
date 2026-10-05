#include "AutonomousAcoreCompat.h"
#include "AutonomousSocialManager.h"

#include "AutonomousBotMgr.h"
#include "AutonomousBotController.h"
#include "AutonomousQuestExecutor.h"
#include "AutonomousRelationshipManager.h"
#include "ObjectMgr.h"
#include "Group.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "MotionMaster.h"
#include "Map.h"
#include "PetDefines.h"

#include <string>


namespace AutonomousAI
{
    namespace
    {
        constexpr float FORM_RANGE = 45.0f;
        constexpr float FOLLOW_RANGE = 28.0f;
        constexpr float BREAK_RANGE = 140.0f;
        constexpr uint32 THINK_INTERVAL = 5000;

        bool IsTankClass(uint8 classId)
        {
            return classId == CLASS_WARRIOR || classId == CLASS_PALADIN || classId == CLASS_DEATH_KNIGHT || classId == CLASS_DRUID;
        }

        bool IsHealerClass(uint8 classId)
        {
            return classId == CLASS_PRIEST || classId == CLASS_SHAMAN || classId == CLASS_PALADIN || classId == CLASS_DRUID;
        }

        bool IsAutonomousPlayer(Player* player)
        {
            return player && sAutonomousBotMgr.HasProfile(player->GetGUID().GetRawValue()) &&
                sAutonomousBotMgr.IsHeadless(player->GetGUID().GetRawValue());
        }
    }

    AutonomousSocialManager::~AutonomousSocialManager()
    {
        Shutdown();
    }

    bool AutonomousSocialManager::IsAutonomous(Player* player) const
    {
        return IsAutonomousPlayer(player);
    }

    bool AutonomousSocialManager::CanJoin(Player* player) const
    {
        return IsAutonomous(player) && player->IsInWorld() && player->IsAlive() && !player->GetGroup();
    }

    Player* AutonomousSocialManager::FindNearbyPartner(Player* leader) const
    {
        if (!leader || !CanJoin(leader))
            return nullptr;

        Player* best = nullptr;
        float bestScore = 100000.0f;
        for (auto const& profile : sAutonomousBotMgr.GetProfiles())
        {
            if (!profile.enabled || profile.guid == leader->GetGUID().GetRawValue())
                continue;

            Player* candidate = ObjectAccessor::FindPlayer(AutonomousMakeGuid(profile.guid));
            if (!CanJoin(candidate) || candidate->GetMapId() != leader->GetMapId())
                continue;

            float distance = leader->GetDistance(candidate);
            if (distance > FORM_RANGE)
                continue;

            // Prefer a partner who adds a complementary role.
            float score = distance;
            int32 relationship = sAutonomousRelationshipMgr.GetScore(leader->GetGUID().GetRawValue(), candidate->GetGUID().GetRawValue());
            if (sAutonomousRelationshipMgr.Get(leader->GetGUID().GetRawValue(), candidate->GetGUID().GetRawValue()).preferred)
                score -= 20.0f;
            score -= static_cast<float>(relationship) * 0.10f;
            if (IsTankClass(leader->getClass()) == IsTankClass(candidate->getClass()))
                score += 8.0f;
            if (IsHealerClass(leader->getClass()) == IsHealerClass(candidate->getClass()))
                score += 5.0f;

            if (score < bestScore)
            {
                bestScore = score;
                best = candidate;
            }
        }

        return best;
    }

    void AutonomousSocialManager::TryFormGroup()
    {
        for (auto const& profile : sAutonomousBotMgr.GetProfiles())
        {
            if (!profile.enabled)
                continue;

            Player* leader = ObjectAccessor::FindPlayer(AutonomousMakeGuid(profile.guid));
            if (!CanJoin(leader))
                continue;

            Player* partner = FindNearbyPartner(leader);
            if (!partner)
                continue;

            auto* group = new Group();
            if (!group->Create(leader))
            {
                delete group;
                continue;
            }

            if (!group->AddMember(partner))
            {
                group->Disband();
                continue;
            }

            // Add one more nearby autonomous player when available. Normal groups
            // are capped by TrinityCore at five members.
            for (auto const& secondProfile : sAutonomousBotMgr.GetProfiles())
            {
                if (group->GetMembersCount() >= MAX_GROUP_SIZE)
                    break;

                Player* candidate = ObjectAccessor::FindPlayer(AutonomousMakeGuid(secondProfile.guid));
                if (!CanJoin(candidate) || candidate->GetMapId() != leader->GetMapId())
                    continue;
                if (leader->GetDistance(candidate) > FORM_RANGE)
                    continue;

                if (!group->AddMember(candidate))
                    break;
            }

            if (leader->GetMap() && leader->GetMap()->IsDungeon())
            {
                for (Group::member_citerator itr = group->GetMemberSlots().begin(); itr != group->GetMemberSlots().end(); ++itr)
                {
                    Player* candidate = ObjectAccessor::FindPlayer(itr->guid);
                    AutonomousBotController* controller = candidate ? sAutonomousBotMgr.Find(candidate->GetGUID().GetRawValue()) : nullptr;
                    if (candidate && controller && std::string(controller->GetRole()) == "tank")
                    {
                        group->ChangeLeader(candidate->GetGUID());
                        break;
                    }
                }
            }

            for (Group::member_citerator itr = group->GetMemberSlots().begin(); itr != group->GetMemberSlots().end(); ++itr)
            {
                for (Group::member_citerator other = group->GetMemberSlots().begin(); other != group->GetMemberSlots().end(); ++other)
                {
                    if (itr->guid != other->guid)
                        sAutonomousRelationshipMgr.RecordInteraction(itr->guid.GetRawValue(), other->guid.GetRawValue(), 1);
                }
            }

            _groups.push_back(group);
            return;
        }
    }

    void AutonomousSocialManager::AssistGroup(Group* group)
    {
        if (!group)
            return;

        Player* leader = ObjectAccessor::FindPlayer(group->GetLeaderGUID());
        if (!leader || !leader->IsInWorld())
            return;

        Unit* victim = leader->GetVictim();
        if (!victim || !victim->IsAlive())
            return;

        for (Group::member_citerator itr = group->GetMemberSlots().begin(); itr != group->GetMemberSlots().end(); ++itr)
        {
            Player* member = ObjectAccessor::FindPlayer(itr->guid);
            if (!member || member == leader || !member->IsAlive())
                continue;

            if (!member->IsValidAttackTarget(victim))
                continue;

            // Inside dungeons the leader controls pulls. Followers only assist the
            // leader's current target, preventing independent bots from body-pulling
            // nearby packs while the party is repositioning.
            if (leader->GetMap() && leader->GetMap()->IsDungeon() && member->GetDistance(leader) > 30.0f)
            {
                member->GetMotionMaster()->MoveFollow(leader, 6.0f, 0.0f);
                continue;
            }

            if (member->GetDistance(victim) <= 45.0f)
                member->Attack(victim, true);
        }
    }

    void AutonomousSocialManager::SyncGroupQuests(Group* group)
    {
        if (!group)
            return;

        Player* leader = ObjectAccessor::FindPlayer(group->GetLeaderGUID());
        if (!leader || !IsAutonomous(leader))
            return;

        AutonomousBotController* leaderController = sAutonomousBotMgr.Find(leader->GetGUID().GetRawValue());
        if (!leaderController)
            return;

        uint32 questId = leaderController->GetActiveQuest();
        if (!questId)
            return;

        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            return;

        for (Group::member_citerator itr = group->GetMemberSlots().begin(); itr != group->GetMemberSlots().end(); ++itr)
        {
            Player* member = ObjectAccessor::FindPlayer(itr->guid);
            if (!member || member == leader || !IsAutonomous(member) || !member->IsAlive())
                continue;

            AutonomousBotController* controller = sAutonomousBotMgr.Find(member->GetGUID().GetRawValue());
            if (!controller || controller->GetActiveQuest())
                continue;

            if (member->GetQuestStatus(questId) != QUEST_STATUS_NONE || member->IsActiveQuest(questId))
                continue;

            if (!member->CanTakeQuest(quest, false) || !member->CanAddQuest(quest, false))
                continue;

            controller->StartQuest(questId);
        }
    }

    void AutonomousSocialManager::FollowLeader(Group* group)
    {
        if (!group)
            return;

        Player* leader = ObjectAccessor::FindPlayer(group->GetLeaderGUID());
        if (!leader || !leader->IsInWorld())
            return;

        for (Group::member_citerator itr = group->GetMemberSlots().begin(); itr != group->GetMemberSlots().end(); ++itr)
        {
            Player* member = ObjectAccessor::FindPlayer(itr->guid);
            if (!member || member == leader || !member->IsAlive() || member->GetVictim())
                continue;

            if (member->GetMapId() != leader->GetMapId())
                continue;

            float distance = member->GetDistance(leader);
            bool dungeonGroup = leader->GetMap() && leader->GetMap()->IsDungeon();
            float followRange = dungeonGroup ? 18.0f : FOLLOW_RANGE;
            if (distance > followRange)
                member->GetMotionMaster()->MoveFollow(leader, dungeonGroup ? 6.0f : 8.0f, 0.0f);
        }
    }

    void AutonomousSocialManager::DestroyGroup(size_t index)
    {
        if (index >= _groups.size())
            return;

        Group* group = _groups[index];
        _groups.erase(_groups.begin() + index);
        if (group)
            group->Disband();
    }

    void AutonomousSocialManager::UpdateGroups()
    {
        for (size_t i = 0; i < _groups.size();)
        {
            Group* group = _groups[i];
            if (!group || group->GetMembersCount() < 2)
            {
                DestroyGroup(i);
                continue;
            }

            Player* leader = ObjectAccessor::FindPlayer(group->GetLeaderGUID());
            if (!leader || !IsAutonomous(leader))
            {
                DestroyGroup(i);
                continue;
            }

            bool tooFar = false;
            bool dungeonGroup = leader->GetMap() && leader->GetMap()->IsDungeon();
            float breakRange = dungeonGroup ? 220.0f : BREAK_RANGE;
            for (Group::member_citerator itr = group->GetMemberSlots().begin(); itr != group->GetMemberSlots().end(); ++itr)
            {
                Player* member = ObjectAccessor::FindPlayer(itr->guid);
                if (!member || !IsAutonomous(member) || !member->IsInWorld() || member->GetMapId() != leader->GetMapId() ||
                    member->GetDistance(leader) > breakRange)
                {
                    tooFar = true;
                    break;
                }
            }

            if (tooFar)
            {
                DestroyGroup(i);
                continue;
            }

            if (dungeonGroup)
            {
                uint32 alive = 0;
                for (Group::member_citerator itr = group->GetMemberSlots().begin(); itr != group->GetMemberSlots().end(); ++itr)
                {
                    Player* member = ObjectAccessor::FindPlayer(itr->guid);
                    if (member && member->IsAlive())
                        ++alive;
                }

                if (!alive)
                {
                    ++i;
                    continue;
                }
            }

            SyncGroupQuests(group);
            AssistGroup(group);
            FollowLeader(group);
            ++i;
        }
    }

    bool AutonomousSocialManager::ShouldDeferDungeonBrain(Player* player) const
    {
        if (!IsAutonomous(player) || !player->IsInWorld() || !player->GetMap() || !player->GetMap()->IsDungeon())
            return false;

        Group* group = player->GetGroup();
        if (!group || group->GetMembersCount() < 2)
            return false;

        return !group->IsLeader(player->GetGUID());
    }

    void AutonomousSocialManager::Update(uint32 diff)
    {
        if (_thinkTimer > diff)
        {
            _thinkTimer -= diff;
            UpdateGroups();
            return;
        }

        _thinkTimer = THINK_INTERVAL;
        UpdateGroups();
        TryFormGroup();
    }

    void AutonomousSocialManager::Shutdown()
    {
        while (!_groups.empty())
            DestroyGroup(_groups.size() - 1);
    }
}

