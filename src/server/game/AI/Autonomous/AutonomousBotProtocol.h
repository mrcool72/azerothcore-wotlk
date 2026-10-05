/*
 * This file is part of the TrinityCore Project. See AUTHORS file for copyright information.
 * Autonomous AI extension.
 */

#ifndef TRINITY_AUTONOMOUS_BOT_PROTOCOL_H
#define TRINITY_AUTONOMOUS_BOT_PROTOCOL_H

#include "Common.h"
#include "ObjectGuid.h"

#include <string>
#include <vector>

namespace AutonomousAI
{
    enum class ActionType : uint8
    {
        NONE = 0,
        MOVE_TO,
        QUEST,
        ACCEPT_QUEST,
        COMPLETE_QUEST,
        ATTACK,
        TALK_TO,
        FOLLOW,
        STOP,
        EXPLORE,
        LOOT,
        TAXI,
        SELL_JUNK,
        REPAIR,
        OPEN_BANK
    };

    enum class GoalType : uint8
    {
        NONE = 0,
        QUEST,
        EXPLORE,
        COMBAT,
        RECOVER,
        SOCIAL
    };

    struct Position
    {
        uint32 mapId = 0;
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float orientation = 0.0f;

        float GetPositionX() const { return x; }
        float GetPositionY() const { return y; }
        float GetPositionZ() const { return z; }
        float GetOrientation() const { return orientation; }
    };

    struct Action
    {
        ActionType type = ActionType::NONE;
        uint32 entry = 0;       // Creature/gameobject entry, or reward choice index.
        uint32 questId = 0;
        uint64 targetGuid = 0;  // Raw ObjectGuid value when a specific world object is required.
        Position destination;
        std::string text;
        uint32 taxiSourceNode = 0;
        uint32 taxiDestinationNode = 0;
    };

    struct QuestInfo
    {
        uint32 questId = 0;
        bool canAccept = false;
        bool canComplete = false;
        uint8 status = 0;
        std::string title;
        std::string giverName;
        uint64 giverGuid = 0;
        uint32 giverEntry = 0;
        float giverDistance = 0.0f;
        int32 questLevel = 0;
        int32 previousQuestId = 0;
        uint32 nextQuestId = 0;
        uint32 nextQuestInChain = 0;
    };

    struct QuestObjectiveInfo
    {
        uint32 questId = 0;
        std::string title;
        uint8 status = 0;
        bool complete = false;
        std::vector<uint32> requiredCreatureOrGameObject;
        std::vector<uint32> requiredCreatureOrGameObjectCount;
        std::vector<uint32> creatureOrGameObjectProgress;
        std::vector<uint32> requiredItems;
        std::vector<uint32> requiredItemCount;
        std::vector<uint32> itemProgress;
        bool explored = false;
    };

    struct WorldObjectInfo
    {
        uint64 guid = 0;
        uint32 entry = 0;
        std::string name;
        Position position;
        float distance = 0.0f;
        uint8 level = 0;
        bool alive = true;
        bool hostile = false;
        bool questGiver = false;
        bool lootAvailable = false;
        bool questObjectiveItem = false;
        bool vendor = false;
        bool banker = false;
        bool innkeeper = false;
        uint32 lootItemCount = 0;
        uint32 lootGold = 0;
    };

    struct GroupMemberInfo
    {
        uint64 guid = 0;
        std::string name;
        std::string role = "damage";
        uint8 level = 1;
        uint32 health = 0;
        uint32 maxHealth = 0;
        uint32 activeQuest = 0;
        float distance = 0.0f;
        bool leader = false;
        bool alive = true;
    };

    struct Perception
    {
        uint64 botGuid = 0;
        uint32 mapId = 0;
        uint8 level = 1;
        uint8 classId = 0;
        std::string role = "damage";
        bool inDungeon = false;
        bool inRaid = false;
        uint32 instanceId = 0;
        uint32 difficulty = 0;
        uint8 groupAlive = 0;
        uint8 groupDead = 0;
        uint8 groupAverageHealth = 100;
        bool groupInCombat = false;
        uint64 dungeonLeaderGuid = 0;
        uint64 dungeonPullTargetGuid = 0;
        bool dungeonPullTargetIsBoss = false;
        bool dungeonRegroupRequired = false;
        bool dungeonWipe = false;
        uint64 bossGuid = 0;
        uint32 bossEntry = 0;
        uint32 bossHealth = 0;
        uint32 bossMaxHealth = 0;
        uint8 bossHealthPercent = 0;
        bool encounterActive = false;
        bool bossDead = false;
        bool bossCasting = false;
        uint32 bossCastSpellId = 0;
        uint32 bossCastTimeMs = 0;
        std::string dungeonState = "none";
        uint32 dungeonCompletedMask = 0;
        uint64 dungeonNextBossGuid = 0;
        uint32 dungeonNextBossEntry = 0;
        float dungeonNextBossDistance = 0.0f;
        bool dungeonCompletionCandidate = false;
        std::string worldRoutine = "idle";
        std::string worldLongTermGoal;
        uint32 worldCompletedQuests = 0;
        uint32 worldLastCompletedQuest = 0;
        uint32 worldVisitedMaps = 0;
        uint32 worldRecommendedQuest = 0;
        uint8 worldGoalProgress = 0;
        uint32 campaignQuest = 0;
        uint32 campaignNextQuest = 0;
        uint32 campaignChainLength = 0;
        uint32 campaignChainProgress = 0;
        std::string campaignState = "idle";
        std::string campaignExecutorState = "idle";
        std::string campaignNavigationState = "idle";
        uint64 campaignTargetGuid = 0;
        uint32 campaignTargetEntry = 0;
        uint32 campaignRetryCount = 0;
        bool campaignStalled = false;
        bool campaignKnowledgeKnown = false;
        uint32 campaignKnowledgeMap = 0;
        Position campaignKnowledgePosition;
        uint32 campaignKnowledgeCount = 0;
        uint32 campaignKnowledgeConfidence = 0;
        uint32 campaignKnowledgeDensity = 0;
        bool routeHasPlan = false;
        std::string routeStage = "idle";
        std::string routeReason = "none";
        Position routeDestination;
        uint32 routeScore = 0;
        uint32 routeDanger = 0;
        uint32 routeConfidence = 0;
        uint32 routeKnownNodes = 0;
        uint32 routeKnownEdges = 0;
        bool routeNeedsTaxi = false;
        uint32 routeTaxiTargetMap = 0;
        std::string routeExecutionState = "idle";
        bool routeStuck = false;
        uint32 routeReplanCount = 0;
        uint32 routeProgressPercent = 0;
        bool navigationHasPlan = false;
        uint32 navigationStep = 0;
        std::string navigationStage = "idle";
        std::string navigationReason = "none";
        Position navigationDestination;
        uint64 resourceCandidateGuid = 0;
        float resourceCandidateDistance = 0.0f;
        std::string resourceCandidateName;
        uint32 resourceCandidateCount = 0;
        bool taxiAvailable = false;
        uint32 taxiSourceNode = 0;
        uint32 taxiDestinationNode = 0;
        uint32 taxiDestinationMap = 0;
        float taxiFlightMasterDistance = 0.0f;
        std::string taxiState = "idle";
        uint32 servicesSoldJunk = 0;
        uint32 servicesRepaired = 0;
        bool servicesBankOpened = false;
        std::string servicesLast = "none";
        bool economyNeedsAttention = false;
        uint8 economyPressure = 0;
        std::string economyReason = "none";
        std::string lifeState = "idle";
        std::string lifeSchedule = "adventure";
        uint32 lifeStateAge = 0;
        uint32 lifeCycle = 0;
        bool traveling = false;
        std::string travelReason = "none";
        Position travelDestination;
        bool needsTown = false;
        uint64 townServiceGuid = 0;
        std::string townServiceType = "none";
        uint32 equipmentUpgradesEquipped = 0;
        uint32 inventoryFreeSlots = 0;
        bool equipmentUpgradePending = false;
        uint32 memoryVisitedMaps = 0;
        uint32 memoryCompletedQuests = 0;
        uint32 memoryLastCompletedQuest = 0;
        uint32 memoryLastMap = 0;
        std::string integrationDecision = "idle";
        uint32 integrationDecisionCount = 0;
        uint64 preferredCompanionGuid = 0;
        std::string preferredCompanionName;
        int32 socialScore = 0;
        uint32 socialInteractions = 0;
        uint64 lootGuid = 0;
        uint32 lootItemCount = 0;
        uint32 lootGold = 0;
        bool lootAvailable = false;
        bool questObjectiveItem = false;
        bool vendor = false;
        bool banker = false;
        bool innkeeper = false;
        uint32 health = 0;
        uint32 maxHealth = 0;
        uint32 power = 0;
        uint32 maxPower = 0;
        Position position;
        std::vector<QuestInfo> quests;
        std::vector<QuestObjectiveInfo> activeQuests;
        std::vector<WorldObjectInfo> nearbyCreatures;
        std::vector<WorldObjectInfo> nearbyGameObjects;
        std::vector<WorldObjectInfo> nearbyPlayers;
        std::vector<GroupMemberInfo> groupMembers;
    };

    inline ObjectGuid AutonomousMakeGuid(uint64 raw)
    {
        return ObjectGuid(raw);
    }

    char const* ToString(ActionType type);
    char const* ToString(GoalType type);

    std::string EscapeJson(std::string const& value);

    // Newline-delimited JSON wire format.
    std::string SerializePerception(Perception const& perception);
    bool ParseAction(std::string const& json, Action& action);
}

#endif
