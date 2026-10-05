/*
 * Autonomous AI protocol serialization.
 */

#include "AutonomousBotProtocol.h"

#ifdef BOOST_BIND_NO_PLACEHOLDERS
#undef BOOST_BIND_NO_PLACEHOLDERS
#endif

#include <boost/bind/bind.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/bind/bind.hpp>

#include <iomanip>
#include <sstream>

namespace AutonomousAI
{
    char const* ToString(ActionType type)
    {
        switch (type)
        {
            case ActionType::MOVE_TO:        return "move_to";
            case ActionType::QUEST:          return "quest";
            case ActionType::ACCEPT_QUEST:   return "accept_quest";
            case ActionType::COMPLETE_QUEST: return "complete_quest";
            case ActionType::ATTACK:         return "attack";
            case ActionType::TALK_TO:        return "talk_to";
            case ActionType::FOLLOW:         return "follow";
            case ActionType::STOP:           return "stop";
            case ActionType::EXPLORE:        return "explore";
            case ActionType::LOOT:           return "loot";
            case ActionType::TAXI:           return "taxi";
            case ActionType::SELL_JUNK:      return "sell_junk";
            case ActionType::REPAIR:         return "repair";
            case ActionType::OPEN_BANK:      return "open_bank";
            default:                          return "none";
        }
    }

    char const* ToString(GoalType type)
    {
        switch (type)
        {
            case GoalType::QUEST:    return "quest";
            case GoalType::EXPLORE:  return "explore";
            case GoalType::COMBAT:   return "combat";
            case GoalType::RECOVER:  return "recover";
            case GoalType::SOCIAL:   return "social";
            default:                 return "none";
        }
    }

    std::string EscapeJson(std::string const& value)
    {
        std::ostringstream out;
        for (char c : value)
        {
            switch (c)
            {
                case '\\': out << "\\\\"; break;
                case '"':  out << "\\\""; break;
                case '\n': out << "\\n"; break;
                case '\r': out << "\\r"; break;
                case '\t': out << "\\t"; break;
                default:   out << c; break;
            }
        }
        return out.str();
    }

    std::string SerializePerception(Perception const& p)
    {
        auto serializePosition = [](Position const& value)
        {
            boost::property_tree::ptree position;
            position.put("map_id", value.mapId);
            position.put("x", value.x);
            position.put("y", value.y);
            position.put("z", value.z);
            position.put("orientation", value.orientation);
            return position;
        };

        boost::property_tree::ptree root;
        root.put("type", "perception");
        root.put("bot_guid", p.botGuid);
        root.put("map_id", p.mapId);
        root.put("level", p.level);
        root.put("class_id", p.classId);
        root.put("role", p.role);
        root.put("instance.in_dungeon", p.inDungeon);
        root.put("instance.in_raid", p.inRaid);
        root.put("instance.id", p.instanceId);
        root.put("instance.difficulty", p.difficulty);
        root.put("group.alive", p.groupAlive);
        root.put("group.dead", p.groupDead);
        root.put("group.average_health", p.groupAverageHealth);
        root.put("group.in_combat", p.groupInCombat);
        root.put("dungeon.leader_guid", p.dungeonLeaderGuid);
        root.put("dungeon.pull_target_guid", p.dungeonPullTargetGuid);
        root.put("dungeon.pull_target_is_boss", p.dungeonPullTargetIsBoss);
        root.put("dungeon.regroup_required", p.dungeonRegroupRequired);
        root.put("dungeon.wipe", p.dungeonWipe);
        root.put("dungeon.state", p.dungeonState);
        root.put("dungeon.boss_cast_spell_id", p.bossCastSpellId);
        root.put("dungeon.boss_cast_time_ms", p.bossCastTimeMs);
        root.put("dungeon.completed_mask", p.dungeonCompletedMask);
        root.put("dungeon.next_boss_guid", p.dungeonNextBossGuid);
        root.put("dungeon.next_boss_entry", p.dungeonNextBossEntry);
        root.put("dungeon.next_boss_distance", p.dungeonNextBossDistance);
        root.put("dungeon.completion_candidate", p.dungeonCompletionCandidate);
        root.put("world.routine", p.worldRoutine);
        root.put("world.long_term_goal", p.worldLongTermGoal);
        root.put("world.completed_quests", p.worldCompletedQuests);
        root.put("world.last_completed_quest", p.worldLastCompletedQuest);
        root.put("world.visited_maps", p.worldVisitedMaps);
        root.put("world.recommended_quest", p.worldRecommendedQuest);
        root.put("world.goal_progress", p.worldGoalProgress);
    root.put("campaign.quest", p.campaignQuest);
    root.put("campaign.next_quest", p.campaignNextQuest);
    root.put("campaign.chain_length", p.campaignChainLength);
    root.put("campaign.chain_progress", p.campaignChainProgress);
    root.put("campaign.state", p.campaignState);
    root.put("campaign.executor_state", p.campaignExecutorState);
    root.put("campaign.navigation_state", p.campaignNavigationState);
    root.put("campaign.target_guid", p.campaignTargetGuid);
    root.put("campaign.target_entry", p.campaignTargetEntry);
    root.put("campaign.retry_count", p.campaignRetryCount);
    root.put("campaign.stalled", p.campaignStalled);
    root.put("campaign.knowledge_known", p.campaignKnowledgeKnown);
    root.put("campaign.knowledge_map", p.campaignKnowledgeMap);
    root.put("campaign.knowledge_x", p.campaignKnowledgePosition.x);
    root.put("campaign.knowledge_y", p.campaignKnowledgePosition.y);
    root.put("campaign.knowledge_z", p.campaignKnowledgePosition.z);
    root.put("campaign.knowledge_count", p.campaignKnowledgeCount);
    root.put("campaign.knowledge_confidence", p.campaignKnowledgeConfidence);
    root.put("campaign.knowledge_density", p.campaignKnowledgeDensity);
    root.put("route.has_plan", p.routeHasPlan);
    root.put("route.stage", p.routeStage);
    root.put("route.reason", p.routeReason);
    root.add_child("route.destination", serializePosition(p.routeDestination));
    root.put("route.score", p.routeScore);
    root.put("route.danger", p.routeDanger);
    root.put("route.confidence", p.routeConfidence);
    root.put("route.known_nodes", p.routeKnownNodes);
    root.put("route.known_edges", p.routeKnownEdges);
    root.put("route.needs_taxi", p.routeNeedsTaxi);
    root.put("route.taxi_target_map", p.routeTaxiTargetMap);
    root.put("route.execution_state", p.routeExecutionState);
    root.put("route.stuck", p.routeStuck);
    root.put("route.replan_count", p.routeReplanCount);
    root.put("route.progress_percent", p.routeProgressPercent);
        root.put("navigation.has_plan", p.navigationHasPlan);
        root.put("navigation.step", p.navigationStep);
        root.put("navigation.stage", p.navigationStage);
        root.put("navigation.reason", p.navigationReason);
        root.add_child("navigation.destination", serializePosition(p.navigationDestination));
        root.put("resources.candidate_guid", p.resourceCandidateGuid);
        root.put("resources.candidate_distance", p.resourceCandidateDistance);
        root.put("resources.candidate_name", p.resourceCandidateName);
        root.put("resources.candidate_count", p.resourceCandidateCount);
        root.put("taxi.available", p.taxiAvailable);
        root.put("taxi.source_node", p.taxiSourceNode);
        root.put("taxi.destination_node", p.taxiDestinationNode);
        root.put("taxi.destination_map", p.taxiDestinationMap);
        root.put("taxi.flight_master_distance", p.taxiFlightMasterDistance);
        root.put("taxi.state", p.taxiState);
        root.put("services.sold_junk", p.servicesSoldJunk);
        root.put("services.repaired", p.servicesRepaired);
        root.put("services.bank_opened", p.servicesBankOpened);
        root.put("services.last", p.servicesLast);
        root.put("economy.needs_attention", p.economyNeedsAttention);
        root.put("economy.pressure", p.economyPressure);
        root.put("economy.reason", p.economyReason);
        root.put("life.state", p.lifeState);
        root.put("life.schedule", p.lifeSchedule);
        root.put("life.state_age_ms", p.lifeStateAge);
        root.put("life.cycle", p.lifeCycle);
        root.put("integration.decision", p.integrationDecision);
        root.put("integration.decision_count", p.integrationDecisionCount);
        root.put("social.preferred_companion_guid", p.preferredCompanionGuid);
        root.put("social.preferred_companion_name", p.preferredCompanionName);
        root.put("social.score", p.socialScore);
        root.put("social.interactions", p.socialInteractions);
        root.put("loot.guid", p.lootGuid);
        root.put("loot.item_count", p.lootItemCount);
        root.put("loot.gold", p.lootGold);
        root.put("loot.available", p.lootAvailable);
        root.put("health", p.health);
        root.put("max_health", p.maxHealth);
        root.put("power", p.power);
        root.put("max_power", p.maxPower);

        boost::property_tree::ptree position;
        position.put("map_id", p.position.mapId);
        position.put("x", p.position.x);
        position.put("y", p.position.y);
        position.put("z", p.position.z);
        position.put("orientation", p.position.orientation);
        root.add_child("position", position);

        root.put("travel.traveling", p.traveling);
        root.put("travel.reason", p.travelReason);
        root.add_child("travel.destination", serializePosition(p.travelDestination));
        root.put("town.needs_town", p.needsTown);
        root.put("town.service_guid", p.townServiceGuid);
        root.put("town.service_type", p.townServiceType);
        root.put("equipment.upgrades_equipped", p.equipmentUpgradesEquipped);
        root.put("equipment.inventory_free_slots", p.inventoryFreeSlots);
        root.put("equipment.upgrade_pending", p.equipmentUpgradePending);
        root.put("memory.visited_maps", p.memoryVisitedMaps);
        root.put("memory.completed_quests", p.memoryCompletedQuests);
        root.put("memory.last_completed_quest", p.memoryLastCompletedQuest);
        root.put("memory.last_map", p.memoryLastMap);

        boost::property_tree::ptree quests;
        for (QuestInfo const& q : p.quests)
        {
            boost::property_tree::ptree item;
            item.put("quest_id", q.questId);
            item.put("can_accept", q.canAccept);
            item.put("can_complete", q.canComplete);
            item.put("status", q.status);
            item.put("title", q.title);
            item.put("giver_name", q.giverName);
            item.put("giver_guid", q.giverGuid);
            item.put("giver_entry", q.giverEntry);
            item.put("giver_distance", q.giverDistance);
            item.put("quest_level", q.questLevel);
            item.put("previous_quest_id", q.previousQuestId);
            item.put("next_quest_id", q.nextQuestId);
            item.put("next_quest_in_chain", q.nextQuestInChain);
            quests.push_back(std::make_pair("", item));
        }
        root.add_child("quests", quests);

        boost::property_tree::ptree activeQuests;
        for (QuestObjectiveInfo const& q : p.activeQuests)
        {
            boost::property_tree::ptree item;
            item.put("quest_id", q.questId);
            item.put("title", q.title);
            item.put("status", q.status);
            item.put("complete", q.complete);
            item.put("explored", q.explored);

            boost::property_tree::ptree requiredCreatureOrGameObject;
            for (uint32 value : q.requiredCreatureOrGameObject)
            {
                boost::property_tree::ptree child;
                child.put("", value);
                requiredCreatureOrGameObject.push_back(std::make_pair("", child));
            }
            item.add_child("required_creature_or_gameobject", requiredCreatureOrGameObject);

            boost::property_tree::ptree requiredCreatureOrGameObjectCount;
            for (uint32 value : q.requiredCreatureOrGameObjectCount)
            {
                boost::property_tree::ptree child;
                child.put("", value);
                requiredCreatureOrGameObjectCount.push_back(std::make_pair("", child));
            }
            item.add_child("required_creature_or_gameobject_count", requiredCreatureOrGameObjectCount);

            boost::property_tree::ptree creatureOrGameObjectProgress;
            for (uint32 value : q.creatureOrGameObjectProgress)
            {
                boost::property_tree::ptree child;
                child.put("", value);
                creatureOrGameObjectProgress.push_back(std::make_pair("", child));
            }
            item.add_child("creature_or_gameobject_progress", creatureOrGameObjectProgress);

            boost::property_tree::ptree requiredItems;
            for (uint32 value : q.requiredItems)
            {
                boost::property_tree::ptree child;
                child.put("", value);
                requiredItems.push_back(std::make_pair("", child));
            }
            item.add_child("required_items", requiredItems);

            boost::property_tree::ptree requiredItemCount;
            for (uint32 value : q.requiredItemCount)
            {
                boost::property_tree::ptree child;
                child.put("", value);
                requiredItemCount.push_back(std::make_pair("", child));
            }
            item.add_child("required_item_count", requiredItemCount);

            boost::property_tree::ptree itemProgress;
            for (uint32 value : q.itemProgress)
            {
                boost::property_tree::ptree child;
                child.put("", value);
                itemProgress.push_back(std::make_pair("", child));
            }
            item.add_child("item_progress", itemProgress);

            activeQuests.push_back(std::make_pair("", item));
        }
        root.add_child("active_quests", activeQuests);

        auto serializeObjects = [&serializePosition](std::vector<WorldObjectInfo> const& objects)
        {
            boost::property_tree::ptree result;
            for (WorldObjectInfo const& object : objects)
            {
                boost::property_tree::ptree item;
                item.put("guid", object.guid);
                item.put("entry", object.entry);
                item.put("name", object.name);
                item.add_child("position", serializePosition(object.position));
                item.put("distance", object.distance);
                item.put("level", object.level);
                item.put("alive", object.alive);
                item.put("hostile", object.hostile);
                item.put("quest_giver", object.questGiver);
                item.put("loot_available", object.lootAvailable);
                item.put("loot_item_count", object.lootItemCount);
                item.put("loot_gold", object.lootGold);
                item.put("vendor", object.vendor);
                item.put("banker", object.banker);
                item.put("innkeeper", object.innkeeper);
                result.push_back(std::make_pair("", item));
            }
            return result;
        };

        root.add_child("nearby_creatures", serializeObjects(p.nearbyCreatures));
        root.add_child("nearby_gameobjects", serializeObjects(p.nearbyGameObjects));
        root.add_child("nearby_players", serializeObjects(p.nearbyPlayers));

        boost::property_tree::ptree groupMembers;
        for (GroupMemberInfo const& member : p.groupMembers)
        {
            boost::property_tree::ptree item;
            item.put("guid", member.guid);
            item.put("name", member.name);
            item.put("role", member.role);
            item.put("level", member.level);
            item.put("health", member.health);
            item.put("max_health", member.maxHealth);
            item.put("active_quest", member.activeQuest);
            item.put("distance", member.distance);
            item.put("leader", member.leader);
            item.put("alive", member.alive);
            groupMembers.push_back(std::make_pair("", item));
        }
        root.add_child("group_members", groupMembers);

        std::ostringstream stream;
        boost::property_tree::write_json(stream, root, false);
        return stream.str();
    }

    bool ParseAction(std::string const& json, Action& action)
    {
        try
        {
            boost::property_tree::ptree root;
            std::istringstream stream(json);
            boost::property_tree::read_json(stream, root);

            std::string const type = root.get<std::string>("action", "none");

            if (type == "move_to")
                action.type = ActionType::MOVE_TO;
            else if (type == "quest")
                action.type = ActionType::QUEST;
            else if (type == "accept_quest")
                action.type = ActionType::ACCEPT_QUEST;
            else if (type == "complete_quest")
                action.type = ActionType::COMPLETE_QUEST;
            else if (type == "attack")
                action.type = ActionType::ATTACK;
            else if (type == "talk_to")
                action.type = ActionType::TALK_TO;
            else if (type == "follow")
                action.type = ActionType::FOLLOW;
            else if (type == "stop")
                action.type = ActionType::STOP;
            else if (type == "explore")
                action.type = ActionType::EXPLORE;
            else if (type == "loot")
                action.type = ActionType::LOOT;
            else if (type == "taxi")
                action.type = ActionType::TAXI;
            else if (type == "sell_junk")
                action.type = ActionType::SELL_JUNK;
            else if (type == "repair")
                action.type = ActionType::REPAIR;
            else if (type == "open_bank")
                action.type = ActionType::OPEN_BANK;
            else
                action.type = ActionType::NONE;

            action.entry = root.get<uint32>("entry", 0);
            action.questId = root.get<uint32>("quest_id", 0);
            action.targetGuid = root.get<uint64>("target_guid", 0);
            action.text = root.get<std::string>("text", "");
            action.taxiSourceNode = root.get<uint32>("taxi_source_node", 0);
            action.taxiDestinationNode = root.get<uint32>("taxi_destination_node", 0);

            if (auto position = root.get_child_optional("destination"))
            {
                action.destination.mapId = position->get<uint32>("map_id", 0);
                action.destination.x = position->get<float>("x", 0.0f);
                action.destination.y = position->get<float>("y", 0.0f);
                action.destination.z = position->get<float>("z", 0.0f);
                action.destination.orientation = position->get<float>("orientation", 0.0f);
            }

            return action.type != ActionType::NONE;
        }
        catch (...)
        {
            return false;
        }
    }
}
