#include "AutonomousWorldManager.h"
#include "AutonomousBotProtocol.h"
#include "Player.h"
#include "QuestDef.h"
#include "AutonomousRoutineManager.h"
#include "AutonomousGoalManager.h"

namespace AutonomousAI
{
    AutonomousWorldManager::AutonomousWorldManager(Player* player) :
        _player(player), _timer(0), _previousActiveQuest(0), _completedQuests(0),
        _lastCompletedQuest(0), _visitedMaps(0), _lastMapId(0), _recommendedQuest(0),
        _routine("idle"), _longTermGoal("build a stable adventure routine"),
        _routineManager(std::make_unique<AutonomousRoutineManager>()),
        _goalManager(std::make_unique<AutonomousGoalManager>()), _goalProgress(0)
    {
    }

    void AutonomousWorldManager::RememberMap(uint32 mapId)
    {
        if (!_maps[mapId])
        {
            _maps[mapId] = true;
            ++_visitedMaps;
        }
        _lastMapId = mapId;
    }

    void AutonomousWorldManager::Update(uint32 diff, Perception const& perception)
    {
        if (!_player || !_player->IsInWorld())
            return;

        RememberMap(perception.mapId);

        uint32 activeQuest = 0;
        for (QuestObjectiveInfo const& quest : perception.activeQuests)
        {
            if (!quest.complete)
            {
                activeQuest = quest.questId;
                break;
            }
        }

        if (_previousActiveQuest && !activeQuest && _player->GetQuestStatus(_previousActiveQuest) == QUEST_STATUS_REWARDED)
        {
            _lastCompletedQuest = _previousActiveQuest;
            ++_completedQuests;
        }
        _previousActiveQuest = activeQuest;

        if (_timer > diff)
        {
            _timer -= diff;
            return;
        }
        _timer = 2500;

        _recommendedQuest = 0;
        float best = 1000000.0f;
        for (QuestInfo const& quest : perception.quests)
        {
            if (!quest.canAccept || !quest.giverGuid)
                continue;

            Quest const* templateQuest = sObjectMgr->GetQuestTemplate(quest.questId);
            if (!templateQuest)
                continue;

            float score = quest.giverDistance;
            if (_lastCompletedQuest)
            {
                if (templateQuest->GetPrevQuestId() == static_cast<int32>(_lastCompletedQuest) ||
                    templateQuest->GetPrevQuestId() == -static_cast<int32>(_lastCompletedQuest))
                    score -= 1000.0f;
            }
            if (templateQuest->GetQuestLevel() > 0)
                score += static_cast<float>(std::abs(templateQuest->GetQuestLevel() - int32(_player->GetLevel()))) * 2.0f;

            if (score < best)
            {
                best = score;
                _recommendedQuest = quest.questId;
            }
        }

        if (_routineManager)
        {
            _routineManager->Update(diff, perception);
            _routine = _routineManager->GetRoutine();
        }

        if (_goalManager)
        {
            _goalManager->Update(diff, perception, _routine);
            _longTermGoal = _goalManager->GetGoal();
            _goalProgress = _goalManager->GetProgress();
        }
    }
}
