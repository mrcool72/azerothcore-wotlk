#ifndef TRINITY_AUTONOMOUS_QUEST_CAMPAIGN_MANAGER_H
#define TRINITY_AUTONOMOUS_QUEST_CAMPAIGN_MANAGER_H

#include "AutonomousBotProtocol.h"
#include <cstdint>
#include <string>

namespace AutonomousAI
{
class AutonomousQuestCampaignManager
{
public:
    AutonomousQuestCampaignManager();
    void Update(uint32 diff, Perception const& perception);
    uint32 GetCampaignQuest() const { return _campaignQuest; }
    uint32 GetNextQuest() const { return _nextQuest; }
    uint32 GetChainLength() const { return _chainLength; }
    uint32 GetChainProgress() const { return _chainProgress; }
    std::string const& GetState() const { return _state; }
    bool HasCampaign() const { return _campaignQuest != 0; }

private:
    void Rebuild(Perception const& perception);
    uint32 Score(QuestInfo const& quest, Perception const& perception) const;

    uint32 _timer = 0;
    uint32 _campaignQuest = 0;
    uint32 _nextQuest = 0;
    uint32 _chainLength = 0;
    uint32 _chainProgress = 0;
    std::string _state = "idle";
};
}

#endif
