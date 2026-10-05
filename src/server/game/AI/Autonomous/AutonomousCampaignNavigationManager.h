#ifndef TRINITY_AUTONOMOUS_CAMPAIGN_NAVIGATION_MANAGER_H
#define TRINITY_AUTONOMOUS_CAMPAIGN_NAVIGATION_MANAGER_H

#include "AutonomousBotProtocol.h"
#include <cstdint>
#include <string>

namespace AutonomousAI
{
class AutonomousCampaignNavigationManager
{
public:
    void Update(uint32 diff, Perception const& perception, bool externalActive);
    bool HasAction() const { return _hasAction; }
    Action ConsumeAction();
    std::string const& GetState() const { return _state; }
    uint32 GetSearchTicks() const { return _searchTicks; }

private:
    void ClearAction();
    void SelectObjectiveAction(Perception const& perception);

    uint32 _timer = 0;
    uint32 _searchTicks = 0;
    bool _hasAction = false;
    Action _action;
    std::string _state = "idle";
};
}

#endif
