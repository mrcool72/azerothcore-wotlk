#include "AutonomousAcoreCompat.h"
/*
 * Autonomous bot controller implementation.
 */

#include "AutonomousBotController.h"
#include "AutonomousQuestCampaignManager.h"
#include "AutonomousCampaignExecutor.h"
#include "AutonomousCampaignNavigationManager.h"
#include "AutonomousQuestKnowledgeManager.h"
#include "AutonomousWorldRouteManager.h"

#include "AutonomousAITransport.h"
#include "AutonomousBotBrain.h"
#include "AutonomousCombatManager.h"
#include "AutonomousDungeonManager.h"
#include "AutonomousWorldManager.h"
#include "AutonomousTravelManager.h"
#include "AutonomousTownManager.h"
#include "AutonomousEquipmentManager.h"
#include "AutonomousMemoryManager.h"
#include "AutonomousNavigationManager.h"
#include "AutonomousResourceManager.h"
#include "AutonomousEconomyManager.h"
#include "AutonomousLifeManager.h"
#include "AutonomousIntegrationManager.h"
#include "AutonomousTaxiManager.h"
#include "AutonomousServiceManager.h"
#include "AutonomousRelationshipManager.h"
#include "AutonomousPlayerAI.h"
#include "AutonomousQuestExecutor.h"
#include "AutonomousBotMgr.h"

#include "CellImpl.h"
#include "Creature.h"
#include "GameObject.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "InstanceScript.h"
#include "Map.h"
#include "Group.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "SharedDefines.h"
#include "QuestDef.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include <set>
#include <vector>

namespace AutonomousAI
{
    AutonomousBotController::AutonomousBotController(Player* player) :
        _player(player),
        _ai(nullptr),
        _actions(),
        _perception(),
        _transport(std::make_unique<ExternalAITransport>()),
        _questExecutor(std::make_unique<AutonomousQuestExecutor>(player)),
        _brain(std::make_unique<AutonomousBotBrain>(player)),
        _combatManager(std::make_unique<AutonomousCombatManager>(player)),
        _dungeonManager(std::make_unique<AutonomousDungeonManager>(player)),
        _worldManager(std::make_unique<AutonomousWorldManager>(player)),
        _travelManager(std::make_unique<AutonomousTravelManager>(player)),
        _townManager(std::make_unique<AutonomousTownManager>(player)),
        _equipmentManager(std::make_unique<AutonomousEquipmentManager>(player)),
        _memoryManager(std::make_unique<AutonomousMemoryManager>(player)),
        _navigationManager(std::make_unique<AutonomousNavigationManager>()),
        _resourceManager(std::make_unique<AutonomousResourceManager>()),
        _economyManager(std::make_unique<AutonomousEconomyManager>()),
        _lifeManager(std::make_unique<AutonomousLifeManager>()),
        _integrationManager(std::make_unique<AutonomousIntegrationManager>()),
        _taxiManager(std::make_unique<AutonomousTaxiManager>(player)),
        _serviceManager(std::make_unique<AutonomousServiceManager>(player)),
        _campaignManager(std::make_unique<AutonomousQuestCampaignManager>()),
        _campaignExecutor(std::make_unique<AutonomousCampaignExecutor>(_player)),
        _campaignNavigationManager(std::make_unique<AutonomousCampaignNavigationManager>()),
        _questKnowledgeManager(&sAutonomousQuestKnowledgeMgr),
        _worldRouteManager(std::make_unique<AutonomousWorldRouteManager>(_player)),
        _externalAIEnabled(false),
        _requestTimer(0)
    {
    }

    AutonomousBotController::~AutonomousBotController()
    {
        StopExternalAI();
    }

    void AutonomousBotController::Update(uint32 diff)
    {
        if (!_player || !_player->IsInWorld())
            return;

        RefreshPerception();

        if (_questExecutor)
            _questExecutor->Update(diff, _perception);

        if (_combatManager)
            _combatManager->Update(diff);

        bool externalActive = _externalAIEnabled && _transport && _transport->IsRunning();

        if (_dungeonManager)
            _dungeonManager->Update(diff, _perception, externalActive);

        if (_worldManager)
            _worldManager->Update(diff, _perception);

        if (_travelManager)
            _travelManager->Update(diff, _perception, externalActive);
        if (_townManager)
            _townManager->Update(diff, _perception, externalActive);
        if (_equipmentManager)
            _equipmentManager->Update(diff, externalActive);
        if (_navigationManager)
            _navigationManager->Update(diff, _perception);
        if (_resourceManager)
            _resourceManager->Update(diff, _perception);
        if (_economyManager)
            _economyManager->Update(diff, _perception);
        if (_taxiManager)
            _taxiManager->Update(diff, _perception, externalActive);
        if (_serviceManager)
            _serviceManager->Update(diff, _perception, externalActive);
        if (_campaignManager)
            _campaignManager->Update(diff, _perception);

        if (_questKnowledgeManager)
            _questKnowledgeManager->Update(diff, _perception);

        if (_worldRouteManager)
        {
            _perception.routeHasPlan = _worldRouteManager->HasPlan();
            _perception.routeStage = _worldRouteManager->GetPlanStage();
            _perception.routeReason = _worldRouteManager->GetPlanReason();
            _perception.routeDestination = _worldRouteManager->GetPlanDestination();
            _perception.routeScore = _worldRouteManager->GetPlanScore();
            _perception.routeDanger = _worldRouteManager->GetCurrentDanger();
            _perception.routeConfidence = _worldRouteManager->GetRouteConfidence();
            _perception.routeKnownNodes = _worldRouteManager->GetKnownNodes();
            _perception.routeKnownEdges = _worldRouteManager->GetKnownEdges();
            _perception.routeNeedsTaxi = _worldRouteManager->NeedsTaxi();
            _perception.routeTaxiTargetMap = _worldRouteManager->GetTaxiTargetMap();
            _perception.routeExecutionState = _worldRouteManager->GetExecutionState();
            _perception.routeStuck = _worldRouteManager->IsStuck();
            _perception.routeReplanCount = _worldRouteManager->GetReplanCount();
            _perception.routeProgressPercent = _worldRouteManager->GetProgressPercent();
        }

        if (_campaignExecutor)
            _campaignExecutor->Update(diff, _perception,
                _campaignManager ? _campaignManager->GetCampaignQuest() : 0,
                _questExecutor && _questExecutor->IsActive(), externalActive);

        if (_campaignNavigationManager)
            _campaignNavigationManager->Update(diff, _perception, externalActive);

        if (_worldRouteManager)
            _worldRouteManager->Update(diff, _perception);

        if (_worldRouteManager && !externalActive && _worldRouteManager->HasPlan())
        {
            if (_worldRouteManager->NeedsTaxi())
                _taxiManager->RequestCampaignTravel(_worldRouteManager->GetTaxiTargetMap());
            else if (_worldRouteManager->GetPlanDestination().mapId == _player->GetMapId() &&
                     _player->GetDistance(_worldRouteManager->GetPlanDestination().x,
                         _worldRouteManager->GetPlanDestination().y,
                         _worldRouteManager->GetPlanDestination().z) > 8.0f)
                _player->GetMotionMaster()->MovePoint(0,
                    _worldRouteManager->GetPlanDestination().x,
                    _worldRouteManager->GetPlanDestination().y,
                    _worldRouteManager->GetPlanDestination().z);
        }

        if (_worldManager)
        {
            _perception.worldRoutine = _worldManager->GetRoutine();
            _perception.worldLongTermGoal = _worldManager->GetLongTermGoal();
            _perception.worldCompletedQuests = _worldManager->GetCompletedQuestCount();
            _perception.worldLastCompletedQuest = _worldManager->GetLastCompletedQuest();
            _perception.worldVisitedMaps = _worldManager->GetVisitedMapCount();
            _perception.worldRecommendedQuest = _worldManager->GetRecommendedQuest();
            _perception.worldGoalProgress = _worldManager->GetGoalProgress();
        }

        if (_travelManager)
        {
            _perception.traveling = _travelManager->IsTraveling();
            _perception.travelDestination = _travelManager->GetDestination();
            _perception.travelReason = _travelManager->GetReason();
        }
        if (_townManager)
        {
            _perception.needsTown = _townManager->NeedsTown();
            _perception.townServiceGuid = _townManager->GetServiceGuid();
            _perception.townServiceType = _townManager->GetServiceType();
        }
        if (_equipmentManager)
        {
            _perception.equipmentUpgradesEquipped = _equipmentManager->GetUpgradesEquipped();
            _perception.inventoryFreeSlots = _equipmentManager->GetInventorySpace();
            _perception.equipmentUpgradePending = _equipmentManager->HasUpgradePending();
        }
        if (_memoryManager)
        {
            _memoryManager->Update(diff, _perception.mapId, _perception.worldRecommendedQuest,
                _perception.worldLastCompletedQuest, _perception.worldCompletedQuests,
                _perception.worldRoutine, _perception.worldLongTermGoal);
            _perception.memoryVisitedMaps = _memoryManager->GetVisitedMaps();
            _perception.memoryCompletedQuests = _memoryManager->GetCompletedQuests();
            _perception.memoryLastCompletedQuest = _memoryManager->GetLastCompletedQuest();
            _perception.memoryLastMap = _memoryManager->GetLastMap();
        }
        if (_navigationManager)
        {
            _perception.navigationHasPlan = _navigationManager->HasPlan();
            _perception.navigationStep = _navigationManager->GetStep();
            _perception.navigationStage = _navigationManager->GetStage();
            _perception.navigationReason = _navigationManager->GetReason();
            _perception.navigationDestination = _navigationManager->GetDestination();
        }
        if (_resourceManager)
        {
            _perception.resourceCandidateGuid = _resourceManager->GetCandidateGuid();
            _perception.resourceCandidateDistance = _resourceManager->GetCandidateDistance();
            _perception.resourceCandidateName = _resourceManager->GetCandidateName();
            _perception.resourceCandidateCount = _resourceManager->GetCandidateCount();
        }
        if (_economyManager)
        {
            _perception.economyNeedsAttention = _economyManager->NeedsEconomy();
            _perception.economyPressure = _economyManager->GetPressure();
            _perception.economyReason = _economyManager->GetReason();
        }
        if (_taxiManager)
        {
            _perception.taxiAvailable = _taxiManager->HasTaxiRoute();
            _perception.taxiSourceNode = _taxiManager->GetSourceNode();
            _perception.taxiDestinationNode = _taxiManager->GetDestinationNode();
            _perception.taxiDestinationMap = _taxiManager->GetDestinationMap();
            _perception.taxiFlightMasterDistance = _taxiManager->GetFlightMasterDistance();
            _perception.taxiState = _taxiManager->GetState();
        }
        if (_serviceManager)
        {
            _perception.servicesSoldJunk = _serviceManager->GetSoldJunk();
            _perception.servicesRepaired = _serviceManager->GetRepaired();
            _perception.servicesBankOpened = _serviceManager->GetBankOpened();
            _perception.servicesLast = _serviceManager->GetLastService();
        }

        if (_campaignManager)
        {
            _perception.campaignQuest = _campaignManager->GetCampaignQuest();
            _perception.campaignNextQuest = _campaignManager->GetNextQuest();
            _perception.campaignChainLength = _campaignManager->GetChainLength();
            _perception.campaignChainProgress = _campaignManager->GetChainProgress();
            _perception.campaignState = _campaignManager->GetState();
        }
        if (_campaignExecutor)
        {
            _perception.campaignExecutorState = _campaignExecutor->GetState();
            _perception.campaignTargetGuid = _campaignExecutor->GetTargetGuid();
            _perception.campaignTargetEntry = _campaignExecutor->GetTargetEntry();
            _perception.campaignRetryCount = _campaignExecutor->GetRetryCount();
            _perception.campaignStalled = _campaignExecutor->IsStalled();
        }
        _perception.campaignKnowledgeKnown = false;
        _perception.campaignKnowledgeMap = 0;
        _perception.campaignKnowledgePosition = Position();
        _perception.campaignKnowledgeConfidence = 0;
        _perception.campaignKnowledgeDensity = 0;
        if (_questKnowledgeManager)
        {
            QuestWorldKnowledge knowledge;
            if (_campaignExecutor && _campaignExecutor->GetTargetEntry() &&
                _questKnowledgeManager->FindBest(_perception.campaignQuest, _campaignExecutor->GetTargetEntry(), _perception.mapId, &_perception.position, knowledge))
            {
                _perception.campaignKnowledgeKnown = true;
                _perception.campaignKnowledgeMap = knowledge.mapId;
                _perception.campaignKnowledgePosition = knowledge.position;
            }
            _perception.campaignKnowledgeCount = _questKnowledgeManager->GetLocationCount(_perception.campaignQuest, _campaignExecutor ? _campaignExecutor->GetTargetEntry() : 0);
                _perception.campaignKnowledgeConfidence = knowledge.confidence;
                _perception.campaignKnowledgeDensity = knowledge.density;
        }
        if (_campaignNavigationManager)
        {
            _perception.campaignNavigationState = _campaignNavigationManager->GetState();
            if (!_externalAIEnabled && _taxiManager && !_perception.taxiAvailable &&
                _campaignNavigationManager->GetSearchTicks() >= 20 &&
                (_perception.campaignNavigationState == "target_not_visible" ||
                 _perception.campaignNavigationState == "searching_objective"))
                _taxiManager->RequestCampaignTravel(_perception.campaignKnowledgeKnown ? _perception.campaignKnowledgeMap : 0);
        }

        if (_lifeManager)
        {
            _lifeManager->Update(diff, _perception, _perception.worldRoutine, _perception.worldLongTermGoal);
            _perception.lifeState = _lifeManager->GetState();
            _perception.lifeSchedule = _lifeManager->GetSchedule();
            _perception.lifeStateAge = _lifeManager->GetStateAge();
            _perception.lifeCycle = _lifeManager->GetCycle();
        }

        if (_integrationManager)
        {
            _integrationManager->Update(diff, _player, _perception, externalActive);
            _perception.integrationDecision = _integrationManager->GetDecision();
            _perception.integrationDecisionCount = _integrationManager->GetDecisionCount();
            if (_integrationManager->HasLocalAction())
                PushAction(_integrationManager->GetAction());
        }

        _perception.preferredCompanionGuid = sAutonomousRelationshipMgr.GetPreferredCompanion(_player->GetGUID().GetRawValue());
        _perception.socialScore = sAutonomousRelationshipMgr.GetSocialScore(_player->GetGUID().GetRawValue());
        _perception.socialInteractions = sAutonomousRelationshipMgr.GetInteractionCount(_player->GetGUID().GetRawValue());
        if (_perception.preferredCompanionGuid)
        {
            if (Player* companion = ObjectAccessor::FindPlayer(([&]() { ObjectGuid g; return ObjectGuid(_perception.preferredCompanionGuid); })()))
                _perception.preferredCompanionName = companion->GetName();
        }

        _perception.role = _combatManager ? _combatManager->GetRoleName() : "damage";

        bool deferDungeonBrain = sAutonomousBotMgr.ShouldDeferDungeonBrain(_player);
        if (_brain && !externalActive && !deferDungeonBrain)
        {
            _brain->Update(diff, _perception, _questExecutor && _questExecutor->IsActive());
            if (uint32 questId = _brain->ConsumeQuestRequest())
                StartQuest(questId);
        }

        if (_campaignNavigationManager && _campaignNavigationManager->HasAction())
            PushAction(_campaignNavigationManager->ConsumeAction());

        Action action;
        while (PopAction(action))
            ExecuteAction(action);

        if (_externalAIEnabled && _transport)
        {
            if (_requestTimer > diff)
                _requestTimer -= diff;
            else
            {
                _requestTimer = 1500;
                SubmitPerception();
            }

            while (_transport->TryPopAction(_player->GetGUID().GetRawValue(), action))
                PushAction(action);

            while (PopAction(action))
                ExecuteAction(action);
        }
    }

    uint64 AutonomousBotController::GetPreferredCompanionGuid() const
    {
        return _perception.preferredCompanionGuid;
    }

    void AutonomousBotController::SetRole(std::string role)
    {
        if (_combatManager)
            _combatManager->SetConfiguredRole(std::move(role));
    }

    void AutonomousBotController::PushAction(Action const& action)
    {
        _actions.push_back(action);
    }

    bool AutonomousBotController::PopAction(Action& action)
    {
        if (_actions.empty())
            return false;

        action = _actions.front();
        _actions.pop_front();
        return true;
    }

    void AutonomousBotController::RefreshPerception()
    {
        BuildPerception();
    }

    bool HasQuestRelation(QuestRelationBounds bounds, uint32 questId)
        {
            for (auto itr = bounds.first; itr != bounds.second; ++itr)
                if (itr->second == questId)
                    return true;
            return false;
        }

        constexpr float PERCEPTION_RANGE = 60.0f;
        constexpr float QUEST_GIVER_RANGE = 80.0f;
        constexpr size_t MAX_NEARBY_OBJECTS = 64;

        struct NearbyCreatureCheck
        {
            Player const* player;
            float range;

            bool operator()(Creature* creature) const
            {
                return creature && player->IsWithinDistInMap(creature, range);
            }
        };

        struct NearbyGameObjectCheck
        {
            Player const* player;
            float range;

            bool operator()(GameObject* gameObject) const
            {
                return gameObject && player->IsWithinDistInMap(gameObject, range);
            }
        };

        struct NearbyUnitCheck
        {
            Player const* player;
            float range;

            bool operator()(Unit* unit) const
            {
                return unit && unit != player && player->IsWithinDistInMap(unit, range);
            }
        };

        // These local visitors intentionally avoid Trinity::UnitListSearcher /
        // CreatureListSearcher / GameObjectListSearcher.  Some Windows builds of
        // this older 3.3.5 tree do not instantiate the corresponding template
        // Visit methods in the final worldserver link.
        struct NearbyCreatureVisitor
        {
            Player const* player;
            float range;
            std::vector<Creature*>& result;

            void Visit(CreatureMapType& map)
            {
                for (auto itr = map.begin(); itr != map.end(); ++itr)
                {
                    Creature* creature = itr->GetSource();
                    if (creature && creature->InSamePhase(player) &&
                        player->IsWithinDistInMap(creature, range))
                        result.push_back(creature);
                }
            }

            template <class T>
            void Visit(GridRefMgr<T>&) { }
        };

        struct NearbyGameObjectVisitor
        {
            Player const* player;
            float range;
            std::vector<GameObject*>& result;

            void Visit(GameObjectMapType& map)
            {
                for (auto itr = map.begin(); itr != map.end(); ++itr)
                {
                    GameObject* gameObject = itr->GetSource();
                    if (gameObject && gameObject->InSamePhase(player) &&
                        player->IsWithinDistInMap(gameObject, range))
                        result.push_back(gameObject);
                }
            }

            template <class T>
            void Visit(GridRefMgr<T>&) { }
        };

        struct NearbyPlayerVisitor
        {
            Player const* player;
            float range;
            std::vector<Unit*>& result;

            void Visit(PlayerMapType& map)
            {
                for (auto itr = map.begin(); itr != map.end(); ++itr)
                {
                    Player* other = itr->GetSource();
                    if (other && other != player && other->InSamePhase(player) &&
                        player->IsWithinDistInMap(other, range))
                        result.push_back(other);
                }
            }

            template <class T>
            void Visit(GridRefMgr<T>&) { }
        };

        Position MakePosition(WorldObject const* object)
        {
            Position result;
            result.mapId = object->GetMapId();
            result.x = object->GetPositionX();
            result.y = object->GetPositionY();
            result.z = object->GetPositionZ();
            result.orientation = object->GetOrientation();
            return result;
        }

        WorldObjectInfo MakeCreatureInfo(Player const* player, Creature const* creature)
        {
            WorldObjectInfo result;
            result.guid = creature->GetGUID().GetRawValue();
            result.entry = creature->GetEntry();
            result.name = creature->GetName();
            result.position = MakePosition(creature);
            result.distance = player->GetDistance(creature);
            result.level = creature->GetLevel();
            result.alive = creature->IsAlive();
            result.hostile = player->IsHostileTo(creature);
            result.questGiver = creature->IsQuestGiver();
            result.vendor = creature->IsVendor();
            result.banker = creature->IsBanker();
            result.innkeeper = creature->IsInnkeeper();
            result.lootAvailable = !creature->IsAlive() && !creature->loot.empty();
            if (result.lootAvailable)
            {
                result.lootItemCount = static_cast<uint32>(creature->loot.items.size() + creature->loot.quest_items.size());
                result.lootGold = creature->loot.gold;
            }
            return result;
        }

        WorldObjectInfo MakeGameObjectInfo(Player const* player, GameObject const* gameObject)
        {
            WorldObjectInfo result;
            result.guid = gameObject->GetGUID().GetRawValue();
            result.entry = gameObject->GetEntry();
            result.name = gameObject->GetName();
            result.position = MakePosition(gameObject);
            result.distance = player->GetDistance(gameObject);
            result.alive = gameObject->isSpawned();
            return result;
        }

        WorldObjectInfo MakePlayerInfo(Player const* player, Player const* other)
        {
            WorldObjectInfo result;
            result.guid = other->GetGUID().GetRawValue();
            result.entry = 0;
            result.name = other->GetName();
            result.position = MakePosition(other);
            result.distance = player->GetDistance(other);
            result.level = other->GetLevel();
            result.alive = other->IsAlive();
            result.hostile = player->IsHostileTo(other);
            return result;
        }

        bool IsDungeonBoss(Unit* unit)
        {
            Creature* creature = unit ? unit->ToCreature() : nullptr;
            return creature && creature->GetCreatureTemplate() && creature->GetCreatureTemplate()->rank >= 3;
        }

        void SortAndLimit(std::vector<WorldObjectInfo>& objects)
        {
            std::sort(objects.begin(), objects.end(), [](WorldObjectInfo const& left, WorldObjectInfo const& right)
            {
                return left.distance < right.distance;
            });

            if (objects.size() > MAX_NEARBY_OBJECTS)
                objects.resize(MAX_NEARBY_OBJECTS);
        }

    void AddQuestGiverQuests(Player* player, WorldObject* giver, std::set<uint32>& questIds)
    {
        if (!giver)
            return;

        if (giver->ToCreature())
        {
            auto startRelations = sObjectMgr->GetCreatureQuestRelationBounds(giver->GetEntry());
            for (auto itr = startRelations.first; itr != startRelations.second; ++itr)
                questIds.insert(itr->second);

            auto involvedRelations = sObjectMgr->GetCreatureQuestInvolvedRelationBounds(giver->GetEntry());
            for (auto itr = involvedRelations.first; itr != involvedRelations.second; ++itr)
                questIds.insert(itr->second);
        }
        else if (giver->ToGameObject())
        {
            auto startRelations = sObjectMgr->GetGOQuestRelationBounds(giver->GetEntry());
            for (auto itr = startRelations.first; itr != startRelations.second; ++itr)
                questIds.insert(itr->second);

            auto involvedRelations = sObjectMgr->GetGOQuestInvolvedRelationBounds(giver->GetEntry());
            for (auto itr = involvedRelations.first; itr != involvedRelations.second; ++itr)
                questIds.insert(itr->second);
        }
    }
    void AutonomousBotController::BuildPerception()
    {
        if (!_player)
            return;

        _perception.quests.clear();
        _perception.activeQuests.clear();
        _perception.nearbyCreatures.clear();
        _perception.nearbyGameObjects.clear();
        _perception.nearbyPlayers.clear();
        _perception.groupMembers.clear();
        _perception.dungeonPullTargetGuid = 0;
        _perception.dungeonPullTargetIsBoss = false;
        _perception.dungeonRegroupRequired = false;
        _perception.dungeonWipe = false;
        _perception.dungeonNextBossGuid = 0;
        _perception.dungeonNextBossEntry = 0;
        _perception.dungeonNextBossDistance = 0.0f;
        _perception.dungeonCompletedMask = 0;
        _perception.dungeonCompletionCandidate = false;
        _perception.bossGuid = 0;
        _perception.bossEntry = 0;
        _perception.bossHealth = 0;
        _perception.bossMaxHealth = 0;
        _perception.bossHealthPercent = 0;
        _perception.encounterActive = false;
        _perception.bossDead = false;
        _perception.bossCasting = false;
        _perception.bossCastSpellId = 0;
        _perception.bossCastTimeMs = 0;
        _perception.lootGuid = 0;
        _perception.lootItemCount = 0;
        _perception.lootGold = 0;
        _perception.lootAvailable = false;

        _perception.botGuid = _player->GetGUID().GetRawValue();
        _perception.mapId = _player->GetMapId();
        _perception.level = _player->GetLevel();
        _perception.classId = _player->getClass();
        _perception.health = _player->GetHealth();
        _perception.maxHealth = _player->GetMaxHealth();
        _perception.power = _player->GetPower(_player->getPowerType());
        _perception.maxPower = _player->GetMaxPower(_player->getPowerType());
        _perception.position = MakePosition(_player);

        if (MapEntry const* mapEntry = sMapStore.LookupEntry(_player->GetMapId()))
        {
            _perception.inDungeon = mapEntry->IsDungeon();
            _perception.inRaid = mapEntry->IsRaid();
        }

        if (Map* map = _player->GetMap())
        {
            _perception.instanceId = map->GetInstanceId();
            _perception.difficulty = static_cast<uint32>(map->GetDifficulty());
        }

        if (InstanceMap* instanceMap = _player->GetMap() ? _player->GetMap()->ToInstanceMap() : nullptr)
            _perception.dungeonCompletedMask = instanceMap->GetInstanceScript() ? instanceMap->GetInstanceScript()->GetCompletedEncounterMask() : 0;
        else
            _perception.dungeonCompletedMask = 0;

        if (Group* group = _player->GetGroup())
        {
            uint32 healthSum = 0;
            uint8 healthCount = 0;
            ObjectGuid leaderGuid = group->GetLeaderGUID();
            for (Group::member_citerator itr = group->GetMemberSlots().begin(); itr != group->GetMemberSlots().end(); ++itr)
            {
                Player* member = ObjectAccessor::FindPlayer(itr->guid);
                if (!member || member->GetMapId() != _player->GetMapId())
                    continue;

                if (member->IsAlive())
                    ++_perception.groupAlive;
                else
                    ++_perception.groupDead;
                healthSum += member->GetHealthPct();
                ++healthCount;
                if (member->GetVictim() && member->GetVictim()->IsAlive())
                    _perception.groupInCombat = true;

                GroupMemberInfo info;
                info.guid = member->GetGUID().GetRawValue();
                info.name = member->GetName();
                info.level = member->GetLevel();
                info.health = member->GetHealth();
                info.maxHealth = member->GetMaxHealth();
                info.distance = _player->GetDistance(member);
                info.leader = member->GetGUID() == leaderGuid;
                info.alive = member->IsAlive();

                if (AutonomousBotController* memberController = sAutonomousBotMgr.Find(info.guid))
                {
                    info.role = memberController->GetRole();
                    info.activeQuest = memberController->GetActiveQuest();
                }

                _perception.groupMembers.push_back(std::move(info));
            }

            if (healthCount)
                _perception.groupAverageHealth = static_cast<uint8>(healthSum / healthCount);
        }

        if (_perception.inDungeon && _player->GetGroup())
        {
            Group* group = _player->GetGroup();
            _perception.dungeonLeaderGuid = group->GetLeaderGUID().GetRawValue();

            Player* leader = ObjectAccessor::FindPlayer(group->GetLeaderGUID());
            Unit* pullTarget = leader ? leader->GetVictim() : nullptr;
            if (pullTarget && pullTarget->IsAlive())
            {
                _perception.dungeonPullTargetGuid = pullTarget->GetGUID().GetRawValue();
                _perception.dungeonPullTargetIsBoss = IsDungeonBoss(pullTarget);
                if (_perception.dungeonPullTargetIsBoss)
                {
                    Creature* boss = pullTarget->ToCreature();
                    _perception.bossGuid = pullTarget->GetGUID().GetRawValue();
                    _perception.bossEntry = boss ? boss->GetEntry() : 0;
                    _perception.bossHealth = pullTarget->GetHealth();
                    _perception.bossMaxHealth = pullTarget->GetMaxHealth();
                    _perception.bossHealthPercent = static_cast<uint8>(pullTarget->GetHealthPct());
                    _perception.encounterActive = true;
                    _perception.bossDead = false;
                    _perception.bossCasting = pullTarget->HasUnitState(UNIT_STATE_CASTING);
                    if (_perception.bossCasting)
                    {
                        Spell* currentSpell = pullTarget->GetCurrentSpell(CURRENT_GENERIC_SPELL);
                        if (!currentSpell)
                            currentSpell = pullTarget->GetCurrentSpell(CURRENT_CHANNELED_SPELL);
                        if (currentSpell)
                        {
                            _perception.bossCastSpellId = currentSpell->GetSpellInfo() ? currentSpell->GetSpellInfo()->Id : 0;
                            _perception.bossCastTimeMs = static_cast<uint32>(std::max<int32>(0, currentSpell->GetCastTimeRemaining()));
                        }
                    }
                }
            }

            _perception.dungeonWipe = _perception.groupAlive == 0 && _perception.groupDead > 0;
            _perception.dungeonRegroupRequired = false;
            for (GroupMemberInfo const& member : _perception.groupMembers)
            {
                if (member.alive && member.distance > 24.0f)
                {
                    _perception.dungeonRegroupRequired = true;
                    break;
                }
            }

            if (_perception.dungeonWipe)
                _perception.dungeonState = "wiped";
            else if (_perception.dungeonPullTargetIsBoss)
                _perception.dungeonState = "boss";
            else if (_perception.groupInCombat)
                _perception.dungeonState = "combat";
            else if (_perception.dungeonRegroupRequired)
                _perception.dungeonState = "regroup";
            else
                _perception.dungeonState = "ready";
        }
        else
        {
            _perception.dungeonState = _perception.inDungeon ? "solo" : "none";
        }

        if (_perception.inDungeon && _perception.bossGuid)
        {
            Unit* boss = ObjectAccessor::GetUnit(*_player, ([&]() { ObjectGuid g; return ObjectGuid(_perception.bossGuid); })());
            if (!boss || !boss->IsAlive())
            {
                _perception.bossDead = true;
                _perception.encounterActive = false;
                _perception.dungeonState = "boss_dead";
            }
        }

        std::set<uint32> activeQuestIds;
        for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
        {
            uint32 questId = _player->GetQuestSlotQuestId(slot);
            if (!questId)
                continue;

            activeQuestIds.insert(questId);

            Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
            if (!quest)
                continue;

            QuestObjectiveInfo info;
            info.questId = questId;
            info.title = quest->GetTitle();
            info.status = uint8(_player->GetQuestStatus(questId));
            info.complete = _player->CanCompleteQuest(questId);

            for (uint8 i = 0; i < QUEST_OBJECTIVES_COUNT; ++i)
            {
                info.requiredCreatureOrGameObject.push_back(quest->RequiredNpcOrGo[i]);
                info.requiredCreatureOrGameObjectCount.push_back(quest->RequiredNpcOrGoCount[i]);
                info.creatureOrGameObjectProgress.push_back(
                    _player->GetQuestSlotCounter(slot, i));
            }

            for (uint8 i = 0; i < QUEST_ITEM_OBJECTIVES_COUNT; ++i)
            {
                info.requiredItems.push_back(quest->RequiredItemId[i]);
                info.requiredItemCount.push_back(quest->RequiredItemCount[i]);
                info.itemProgress.push_back(0);
            }

            QuestStatusMap const& questStatusMap = _player->getQuestStatusMap();
            auto statusItr = questStatusMap.find(questId);
            if (statusItr != questStatusMap.end())
            {
                for (uint8 i = 0; i < QUEST_ITEM_OBJECTIVES_COUNT; ++i)
                    info.itemProgress[i] = statusItr->second.ItemCount[i];
                info.explored = statusItr->second.Explored;
            }

            _perception.activeQuests.push_back(std::move(info));
        }

        float perceptionRange = _perception.inDungeon ? 120.0f : PERCEPTION_RANGE;
        std::vector<Creature*> creatures;
        NearbyCreatureVisitor creatureVisitor { _player, perceptionRange, creatures };
        Cell::VisitObjects(_player, creatureVisitor, perceptionRange);

        std::set<uint32> nearbyQuestGiverEntries;
        for (Creature* creature : creatures)
        {
            if (!creature)
                continue;

            _perception.nearbyCreatures.push_back(MakeCreatureInfo(_player, creature));

            if (creature->IsQuestGiver())
                nearbyQuestGiverEntries.insert(creature->GetEntry());
        }

        std::vector<GameObject*> gameObjects;
        NearbyGameObjectVisitor gameObjectVisitor { _player, perceptionRange, gameObjects };
        Cell::VisitObjects(_player, gameObjectVisitor, perceptionRange);

        for (GameObject* gameObject : gameObjects)
        {
            if (gameObject)
                _perception.nearbyGameObjects.push_back(MakeGameObjectInfo(_player, gameObject));
        }

        std::vector<Unit*> units;
        NearbyPlayerVisitor playerVisitor { _player, perceptionRange, units };
        Cell::VisitObjects(_player, playerVisitor, perceptionRange);

        for (Unit* unit : units)
        {
            if (Player* other = unit ? unit->ToPlayer() : nullptr)
                _perception.nearbyPlayers.push_back(MakePlayerInfo(_player, other));
        }

        SortAndLimit(_perception.nearbyCreatures);
        SortAndLimit(_perception.nearbyGameObjects);
        SortAndLimit(_perception.nearbyPlayers);

        for (WorldObjectInfo const& object : _perception.nearbyCreatures)
        {
            Unit* unit = ObjectAccessor::GetUnit(*_player, ([&]() { ObjectGuid g; return ObjectGuid(object.guid); })());
            Creature* creature = unit ? unit->ToCreature() : nullptr;
            if (creature && creature->IsAlive() && creature->GetCreatureTemplate() && creature->GetCreatureTemplate()->rank >= 3)
            {
                _perception.dungeonNextBossGuid = object.guid;
                _perception.dungeonNextBossEntry = object.entry;
                _perception.dungeonNextBossDistance = object.distance;
                break;
            }
        }

        for (WorldObjectInfo const& object : _perception.nearbyCreatures)
        {
            if (object.lootAvailable)
            {
                _perception.lootGuid = object.guid;
                _perception.lootItemCount = object.lootItemCount;
                _perception.lootGold = object.lootGold;
                _perception.lootAvailable = true;
                break;
            }
        }
        _perception.dungeonCompletionCandidate = _perception.inDungeon && _perception.dungeonCompletedMask != 0 &&
            !_perception.encounterActive && !_perception.groupInCombat && !_perception.dungeonNextBossGuid;

        std::set<uint32> nearbyQuestIds;
        std::vector<WorldObject*> questGivers;

        for (Creature* creature : creatures)
        {
            if (creature && creature->IsQuestGiver() &&
                _player->IsWithinDistInMap(creature, QUEST_GIVER_RANGE))
            {
                questGivers.push_back(creature);
                AddQuestGiverQuests(_player, creature, nearbyQuestIds);
            }
        }

        for (GameObject* gameObject : gameObjects)
        {
            if (!gameObject)
                continue;

            auto startRelations = sObjectMgr->GetGOQuestRelationBounds(gameObject->GetEntry());
            auto involvedRelations = sObjectMgr->GetGOQuestInvolvedRelationBounds(gameObject->GetEntry());
            if (startRelations.first != startRelations.second ||
                involvedRelations.first != involvedRelations.second)
            {
                questGivers.push_back(gameObject);
                AddQuestGiverQuests(_player, gameObject, nearbyQuestIds);
            }
        }

        for (uint32 questId : nearbyQuestIds)
        {
            Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
            if (!quest)
                continue;

            QuestInfo info;
            info.questId = questId;
            info.title = quest->GetTitle();
            info.status = uint8(_player->GetQuestStatus(questId));
            info.canAccept = _player->CanTakeQuest(quest, false) && !_player->IsActiveQuest(questId);
            info.canComplete = _player->CanCompleteQuest(questId);

            WorldObject* nearestGiver = nullptr;
            float nearestDistance = std::numeric_limits<float>::max();

            for (WorldObject* giver : questGivers)
            {
                bool related = false;
                if (Creature* creature = giver->ToCreature())
                {
                    auto start = sObjectMgr->GetCreatureQuestRelationBounds(creature->GetEntry());
                    auto involved = sObjectMgr->GetCreatureQuestInvolvedRelationBounds(creature->GetEntry());
                    related = std::any_of(start.first, start.second, [&](auto const& relation) { return relation.second == questId; }) ||
                        std::any_of(involved.first, involved.second, [&](auto const& relation) { return relation.second == questId; });
                }
                else if (GameObject* gameObject = giver->ToGameObject())
                {
                    auto start = sObjectMgr->GetGOQuestRelationBounds(gameObject->GetEntry());
                    auto involved = sObjectMgr->GetGOQuestInvolvedRelationBounds(gameObject->GetEntry());
                    related = std::any_of(start.first, start.second, [&](auto const& relation) { return relation.second == questId; }) ||
                        std::any_of(involved.first, involved.second, [&](auto const& relation) { return relation.second == questId; });
                }

                if (!related)
                    continue;

                float distance = _player->GetDistance(giver);
                if (distance < nearestDistance)
                {
                    nearestDistance = distance;
                    nearestGiver = giver;
                }
            }

            if (nearestGiver)
            {
                info.giverGuid = nearestGiver->GetGUID().GetRawValue();
                info.giverEntry = nearestGiver->GetEntry();
                info.giverDistance = nearestDistance;
                info.giverName = nearestGiver->GetName();
            }

            if (Quest const* questTemplate = sObjectMgr->GetQuestTemplate(questId))
            {
                info.questLevel = questTemplate->GetQuestLevel();
                info.previousQuestId = questTemplate->GetPrevQuestId();
                info.nextQuestId = questTemplate->GetNextQuestId();
                info.nextQuestInChain = questTemplate->GetNextQuestInChain();
            }

            _perception.quests.push_back(std::move(info));
        }

        (void)activeQuestIds;
        (void)nearbyQuestGiverEntries;
    }

    void AutonomousBotController::SubmitPerception()
    {
        if (!_transport || !_transport->IsRunning())
            return;

        _transport->Submit(_perception.botGuid, SerializePerception(_perception));
    }

    void AutonomousBotController::SetPersonality(std::string personality)
    {
        if (_brain)
            _brain->SetPersonality(std::move(personality));
    }

    void AutonomousBotController::ConfigureExternalAI(std::string host, uint16 port, std::string token)
    {
        if (_transport)
            _transport->Configure(std::move(host), port, std::move(token));
    }

    void AutonomousBotController::SetExternalAIRequestInterval(uint32 milliseconds)
    {
        _requestTimer = milliseconds;
    }

    bool AutonomousBotController::StartExternalAI()
    {
        if (!_transport)
            return false;

        _externalAIEnabled = true;
        _requestTimer = 0;
        return _transport->Start();
    }

    void AutonomousBotController::StopExternalAI()
    {
        _externalAIEnabled = false;
        if (_transport)
            _transport->Stop();
    }

    bool AutonomousBotController::StartQuest(uint32 questId)
    {
        return _questExecutor && _questExecutor->Start(questId);
    }

    void AutonomousBotController::CancelQuest()
    {
        if (_questExecutor)
            _questExecutor->Cancel();
    }

    uint32 AutonomousBotController::GetActiveQuest() const
    {
        return _questExecutor ? _questExecutor->GetQuestId() : 0;
    }

    char const* AutonomousBotController::GetQuestState() const
    {
        return _questExecutor ? _questExecutor->GetStateName() : "idle";
    }

    char const* AutonomousBotController::GetRole() const
    {
        return _combatManager ? _combatManager->GetRoleName() : "damage";
    }

    void AutonomousBotController::ExecuteAction(Action const& action)
    {
        if (!_player)
            return;

        switch (action.type)
        {
            case ActionType::QUEST:
                StartQuest(action.questId);
                break;

            case ActionType::MOVE_TO:
                if (action.destination.mapId == _player->GetMapId())
                    _player->GetMotionMaster()->MovePoint(
                        0,
                        action.destination.x,
                        action.destination.y,
                        action.destination.z,
                        FORCED_MOVEMENT_NONE,
                        0.0f,
                        action.destination.orientation);
                break;

            case ActionType::LOOT:
            {
                ObjectGuid guid = AutonomousMakeGuid(action.targetGuid ? action.targetGuid : _perception.lootGuid);
                Creature* creature = ObjectAccessor::GetCreature(*_player, guid);
                if (!creature || creature->IsAlive() || _player->GetDistance(creature) > 6.0f || creature->loot.empty())
                    break;

                _player->SetLootGUID(guid);
                uint32 maxSlot = creature->loot.GetMaxSlotInLootFor(_player);
                for (uint32 slot = 0; slot < maxSlot; ++slot)
                {
                    InventoryResult lootResult;
                    _player->StoreLootItem(static_cast<uint8>(slot), &creature->loot, lootResult);
                }
                if (creature->loot.gold)
                {
                    uint32 gold = creature->loot.gold;
                    creature->loot.gold = 0;
                    _player->ModifyMoney(static_cast<int32>(gold));
                    creature->loot.NotifyMoneyRemoved();
                }
                if (creature->loot.isLooted())
                {
                    creature->AllLootRemovedFromCorpse();
                    _player->SetLootGUID(ObjectGuid::Empty);
                }
                break;
            }

            case ActionType::STOP:
                _player->GetMotionMaster()->Clear();
                break;

            case ActionType::ATTACK:
                if (action.targetGuid)
                {
                    if (Unit* target = ObjectAccessor::GetUnit(*_player, ([&]() { ObjectGuid g; return ObjectGuid(action.targetGuid); })()))
                        if (_player->IsValidAttackTarget(target))
                            _player->Attack(target, true);
                }
                break;

            case ActionType::ACCEPT_QUEST:
            case ActionType::TALK_TO:
            {
                if (!action.questId || !action.targetGuid)
                    break;

                ObjectGuid const guid = AutonomousMakeGuid(action.targetGuid);
                Object* questGiver = nullptr;

                if (Creature* creature = _player->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_QUESTGIVER))
                    questGiver = creature;
                else if (GameObject* gameObject = _player->GetGameObjectIfCanInteractWith(guid, GAMEOBJECT_TYPE_QUESTGIVER))
                    questGiver = gameObject;

                if (!questGiver)
                    break;

                Quest const* quest = sObjectMgr->GetQuestTemplate(action.questId);
                if (!quest)
                    break;

                if (_player->CanTakeQuest(quest, false) && _player->CanAddQuest(quest, false))
                    _player->AddQuestAndCheckCompletion(quest, questGiver);
                else if (action.type == ActionType::TALK_TO &&
                         _player->CanRewardQuest(quest, action.entry, false))
                    _player->RewardQuest(quest, action.entry, questGiver);
                break;
            }

            case ActionType::COMPLETE_QUEST:
            {
                if (!action.questId || !action.targetGuid)
                    break;

                ObjectGuid const guid = AutonomousMakeGuid(action.targetGuid);
                Object* questGiver = nullptr;

                if (Creature* creature = _player->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_QUESTGIVER))
                    questGiver = creature;
                else if (GameObject* gameObject = _player->GetGameObjectIfCanInteractWith(guid, GAMEOBJECT_TYPE_QUESTGIVER))
                    questGiver = gameObject;

                if (!questGiver)
                    break;

                Quest const* quest = sObjectMgr->GetQuestTemplate(action.questId);
                if (quest && _player->CanRewardQuest(quest, action.entry, false))
                    _player->RewardQuest(quest, action.entry, questGiver);
                break;
            }

            case ActionType::TAXI:
                if (_taxiManager && action.taxiSourceNode && action.taxiDestinationNode)
                    _taxiManager->RequestRoute(action.taxiSourceNode, action.taxiDestinationNode);
                break;

            case ActionType::SELL_JUNK:
            case ActionType::REPAIR:
            case ActionType::OPEN_BANK:
                // Service execution is intentionally centralized in the service manager.
                // The external action selects the service; interaction/range checks remain server-side.
                if (_serviceManager)
                {
                    if (action.type == ActionType::SELL_JUNK)
                        _serviceManager->RequestService("vendor", action.targetGuid);
                    else if (action.type == ActionType::REPAIR)
                        _serviceManager->RequestService("repair", action.targetGuid);
                    else
                        _serviceManager->RequestService("bank", action.targetGuid);
                }
                break;

            default:
                break;
        }
    }
}



