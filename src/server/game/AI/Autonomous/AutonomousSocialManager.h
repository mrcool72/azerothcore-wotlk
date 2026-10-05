#ifndef TRINITY_AUTONOMOUS_SOCIAL_MANAGER_H
#define TRINITY_AUTONOMOUS_SOCIAL_MANAGER_H

#include "Common.h"

#include <vector>

class Group;
class Player;

namespace AutonomousAI
{
    class AutonomousSocialManager
    {
    public:
        AutonomousSocialManager() = default;
        ~AutonomousSocialManager();

        AutonomousSocialManager(AutonomousSocialManager const&) = delete;
        AutonomousSocialManager& operator=(AutonomousSocialManager const&) = delete;

        void Update(uint32 diff);
        void Shutdown();

        bool ShouldDeferDungeonBrain(Player* player) const;

    private:
        bool IsAutonomous(Player* player) const;
        bool CanJoin(Player* player) const;
        Player* FindNearbyPartner(Player* leader) const;
        void TryFormGroup();
        void UpdateGroups();
        void AssistGroup(Group* group);
        void FollowLeader(Group* group);
        void SyncGroupQuests(Group* group);
        void DestroyGroup(size_t index);

        uint32 _thinkTimer = 0;
        std::vector<Group*> _groups;
    };
}

#endif
