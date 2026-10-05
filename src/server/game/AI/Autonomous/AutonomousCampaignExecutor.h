#ifndef TRINITY_AUTONOMOUS_CAMPAIGN_EXECUTOR_H
#define TRINITY_AUTONOMOUS_CAMPAIGN_EXECUTOR_H

#include "AutonomousBotProtocol.h"

#include <cstdint>
#include <string>

class Player;

namespace AutonomousAI
{
class AutonomousCampaignExecutor
{
public:
    explicit AutonomousCampaignExecutor(Player* player);

    void Update(uint32 diff, Perception const& perception, uint32 campaignQuest,
        bool questExecutorActive, bool externalActive);

    uint64 GetTargetGuid() const { return _targetGuid; }
    uint32 GetTargetEntry() const { return _targetEntry; }
    std::string const& GetState() const { return _state; }
    uint32 GetRetryCount() const { return _retryCount; }
    bool IsStalled() const { return _stalled; }

private:
    void ResolveTarget(Perception const& perception);
    bool HasProgress(QuestObjectiveInfo const& quest) const;
    void ResetProgress(QuestObjectiveInfo const& quest);

    Player* _player;
    uint32 _campaignQuest = 0;
    uint64 _targetGuid = 0;
    uint32 _targetEntry = 0;
    uint32 _thinkTimer = 0;
    uint32 _stallTimer = 0;
    uint32 _retryTimer = 0;
    uint32 _retryCount = 0;
    uint32 _lastProgress = 0;
    bool _stalled = false;
    std::string _state = "idle";
};
}

#endif
