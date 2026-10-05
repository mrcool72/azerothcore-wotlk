#include "AutonomousQuestCampaignManager.h"
#include "ObjectMgr.h"
#include "QuestDef.h"
#include <algorithm>
#include <set>

namespace AutonomousAI
{
AutonomousQuestCampaignManager::AutonomousQuestCampaignManager() = default;

uint32 AutonomousQuestCampaignManager::Score(QuestInfo const& q, Perception const& perception) const
{
    if (!q.questId)
        return 0;
    uint32 score = 1;
    if (q.canAccept) score += 40;
    if (q.canComplete) score += 120;
    if (q.nextQuestInChain || q.nextQuestId) score += 20;
    if (q.previousQuestId || q.nextQuestInChain) score += 15;
    if (q.giverDistance < 15.0f) score += 15;
    if (q.questLevel > 0 && q.questLevel <= int32(perception.level) + 2) score += 10;
    if (q.questLevel > int32(perception.level) + 5) score = score > 25 ? score - 25 : 1;
    return score;
}

void AutonomousQuestCampaignManager::Update(uint32 diff, Perception const& perception)
{
    if (_timer > diff) { _timer -= diff; return; }
    _timer = 5000;
    Rebuild(perception);
}

void AutonomousQuestCampaignManager::Rebuild(Perception const& perception)
{
    _state = "searching";
    _campaignQuest = 0;
    _nextQuest = 0;
    _chainLength = 0;
    _chainProgress = 0;

    // Continue an active chain whenever its template exposes the next link.
    for (QuestObjectiveInfo const& active : perception.activeQuests)
    {
        Quest const* q = sObjectMgr->GetQuestTemplate(active.questId);
        if (!q) continue;
        _campaignQuest = active.questId;
        _nextQuest = q->GetNextQuestInChain() ? q->GetNextQuestInChain() : (q->GetNextQuestId() > 0 ? uint32(q->GetNextQuestId()) : 0);
        _state = active.complete ? "ready_to_turn_in" : "executing";
        break;
    }

    if (!_campaignQuest)
    {
        uint32 best = 0, bestScore = 0;
        for (QuestInfo const& q : perception.quests)
        {
            uint32 s = Score(q, perception);
            if (s > bestScore) { bestScore = s; best = q.questId; }
        }
        _campaignQuest = best;
        if (best)
        {
            Quest const* q = sObjectMgr->GetQuestTemplate(best);
            _nextQuest = q ? (q->GetNextQuestInChain() ? q->GetNextQuestInChain() : (q->GetNextQuestId() > 0 ? uint32(q->GetNextQuestId()) : 0)) : 0;
            _state = "selected";
        }
    }

    if (!_campaignQuest) { _state = "idle"; return; }

    // Walk the static chain only for metadata; never auto-accept quests that are not visible/valid.
    std::set<uint32> seen;
    uint32 current = _campaignQuest;
    while (current && seen.insert(current).second && _chainLength < 64)
    {
        ++_chainLength;
        Quest const* q = sObjectMgr->GetQuestTemplate(current);
        if (!q) break;
        uint32 next = q->GetNextQuestInChain() ? q->GetNextQuestInChain() : (q->GetNextQuestId() > 0 ? uint32(q->GetNextQuestId()) : 0);
        if (!next) break;
        current = next;
    }

    if (_chainLength)
        _chainProgress = 100 / _chainLength;
    if (_nextQuest == 0 && _state == "executing")
        _chainProgress = 100;
}
}
