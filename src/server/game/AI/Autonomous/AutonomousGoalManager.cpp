#include "AutonomousGoalManager.h"
#include "AutonomousBotProtocol.h"

#include <algorithm>

namespace AutonomousAI
{
    void AutonomousGoalManager::Update(uint32 diff, Perception const& perception, std::string const& routine)
    {
        if (_timer > diff)
        {
            _timer -= diff;
            return;
        }
        _timer = 7500;

        if (routine == "dungeon")
        {
            _goal = perception.dungeonCompletionCandidate ? "finish the dungeon" : "complete the current dungeon objectives";
            _progress = perception.dungeonCompletionCandidate ? 90 : (perception.dungeonCompletedMask ? 50 : 10);
        }
        else if (routine == "recovery")
        {
            _goal = "recover and resume adventuring";
            _progress = perception.maxHealth ? static_cast<uint8>(std::min(100u, perception.health * 100u / perception.maxHealth)) : 0;
        }
        else if (routine == "town")
        {
            _goal = "restock and improve equipment";
            _progress = perception.equipmentUpgradePending ? 25 : 80;
        }
        else if (routine == "social")
        {
            _goal = "stay with the party and assist companions";
            _progress = perception.groupInCombat ? 75 : 50;
        }
        else if (!perception.activeQuests.empty())
        {
            _goal = "complete the active quest chain";
            uint32 total = 0;
            uint32 complete = 0;
            for (QuestObjectiveInfo const& quest : perception.activeQuests)
            {
                size_t count = std::max(quest.requiredCreatureOrGameObject.size(), quest.requiredItems.size());
                total += static_cast<uint32>(count);
                for (size_t i = 0; i < quest.requiredCreatureOrGameObject.size(); ++i)
                    if (i < quest.creatureOrGameObjectProgress.size() && i < quest.requiredCreatureOrGameObjectCount.size() && quest.requiredCreatureOrGameObjectCount[i] && quest.creatureOrGameObjectProgress[i] >= quest.requiredCreatureOrGameObjectCount[i])
                        ++complete;
                for (size_t i = 0; i < quest.requiredItems.size(); ++i)
                    if (i < quest.itemProgress.size() && i < quest.requiredItemCount.size() && quest.requiredItemCount[i] && quest.itemProgress[i] >= quest.requiredItemCount[i])
                        ++complete;
            }
            _progress = total ? static_cast<uint8>(std::min(100u, complete * 100u / total)) : 10;
        }
        else if (perception.worldRecommendedQuest)
        {
            _goal = "continue the quest chain";
            _progress = 20;
        }
        else
        {
            _goal = "explore the current region and find useful quests";
            _progress = static_cast<uint8>(std::min<uint32>(100, perception.worldVisitedMaps * 10));
        }
    }
}
