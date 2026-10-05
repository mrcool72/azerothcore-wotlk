#include "AutonomousAcoreCompat.h"
#include "AutonomousWorldRouteManager.h"
#include "Player.h"
#include "DatabaseEnv.h"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <limits>

namespace AutonomousAI
{
AutonomousWorldRouteManager::AutonomousWorldRouteManager(Player* player) : _player(player) {}

uint32 AutonomousWorldRouteManager::Cell(float coordinate)
{
    return static_cast<uint32>(std::floor(coordinate / 50.0f) + 100000.0f);
}

uint64 AutonomousWorldRouteManager::NodeKey(uint32 mapId, uint32 cellX, uint32 cellY)
{
    uint64 key = static_cast<uint64>(mapId) << 32;
    key ^= static_cast<uint64>(cellX) * 0x9E3779B185EBCA87ULL;
    key ^= static_cast<uint64>(cellY) * 0xC2B2AE3D27D4EB4FULL;
    return key;
}

uint64 AutonomousWorldRouteManager::EdgeKey(uint32 fromMap, uint32 fromCellX, uint32 fromCellY,
    uint32 toMap, uint32 toCellX, uint32 toCellY)
{
    return NodeKey(fromMap, fromCellX, fromCellY) ^
        (NodeKey(toMap, toCellX, toCellY) * 0x165667B19E3779F9ULL);
}

void AutonomousWorldRouteManager::LoadNode(uint32 mapId, uint32 cellX, uint32 cellY)
{
    uint64 key = NodeKey(mapId, cellX, cellY);
    if (_nodes.find(key) != _nodes.end())
        return;

    if (QueryResult result = WorldDatabase.Query(
        "SELECT x, y, z, o, observations, deaths, danger, objective_count, last_seen FROM autonomous_bot_route_nodes WHERE map_id = {} AND cell_x = {} AND cell_y = {}",
        mapId, cellX, cellY))
    {
        Field* f = result->Fetch();
        RouteNodeInfo n;
        n.mapId = mapId; n.cellX = cellX; n.cellY = cellY;
        n.position.mapId = mapId;
        n.position.x = f[0].GetFloat(); n.position.y = f[1].GetFloat();
        n.position.z = f[2].GetFloat(); n.position.orientation = f[3].GetFloat();
        n.observations = f[4].GetUInt32(); n.deaths = f[5].GetUInt32();
        n.danger = f[6].GetUInt32(); n.objectiveCount = f[7].GetUInt32();
        n.lastSeen = f[8].GetUInt64();
        _nodes.emplace(key, n);
    }
}

void AutonomousWorldRouteManager::LoadMapNodes(uint32 mapId)
{
    if (_nodes.size() > 8)
        return;

    if (QueryResult result = WorldDatabase.PQuery(
        "SELECT cell_x, cell_y, x, y, z, o, observations, deaths, danger, objective_count, last_seen FROM autonomous_bot_route_nodes WHERE map_id = {} ORDER BY danger ASC, observations DESC LIMIT 200",
        mapId))
    {
        do
        {
            Field* f = result->Fetch();
            RouteNodeInfo n;
            n.mapId = mapId;
            n.cellX = f[0].GetUInt32();
            n.cellY = f[1].GetUInt32();
            n.position.mapId = mapId;
            n.position.x = f[2].GetFloat(); n.position.y = f[3].GetFloat();
            n.position.z = f[4].GetFloat(); n.position.orientation = f[5].GetFloat();
            n.observations = f[6].GetUInt32(); n.deaths = f[7].GetUInt32();
            n.danger = f[8].GetUInt32(); n.objectiveCount = f[9].GetUInt32();
            n.lastSeen = f[10].GetUInt64();
            _nodes[NodeKey(mapId, n.cellX, n.cellY)] = n;
        } while (result->NextRow());
    }
}

void AutonomousWorldRouteManager::LoadEdges(uint32 mapId, uint32 cellX, uint32 cellY)
{
    (void)cellX;
    (void)cellY;
    if (!_edges.empty())
        return;

    if (QueryResult result = WorldDatabase.PQuery(
        "SELECT from_map, from_cell_x, from_cell_y, to_map, to_cell_x, to_cell_y, traversals, failures, danger, last_used FROM autonomous_bot_route_edges WHERE from_map = {} LIMIT 500",
        mapId))
    {
        do
        {
            Field* f = result->Fetch();
            RouteEdgeInfo e;
            e.fromMap = f[0].GetUInt32(); e.fromCellX = f[1].GetUInt32(); e.fromCellY = f[2].GetUInt32();
            e.toMap = f[3].GetUInt32(); e.toCellX = f[4].GetUInt32(); e.toCellY = f[5].GetUInt32();
            e.traversals = f[6].GetUInt32(); e.failures = f[7].GetUInt32();
            e.danger = f[8].GetUInt32(); e.lastUsed = f[9].GetUInt64();
            _edges[EdgeKey(e.fromMap, e.fromCellX, e.fromCellY, e.toMap, e.toCellX, e.toCellY)] = e;
        } while (result->NextRow());
    }
}

void AutonomousWorldRouteManager::SaveNode(RouteNodeInfo const& n)
{
    WorldDatabase.Execute(
        "REPLACE INTO autonomous_bot_route_nodes (map_id, cell_x, cell_y, x, y, z, o, observations, deaths, danger, objective_count, last_seen) VALUES ({}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {})",
        n.mapId, n.cellX, n.cellY, n.position.x, n.position.y, n.position.z,
        n.position.orientation, n.observations, n.deaths, n.danger, n.objectiveCount, n.lastSeen);
}

void AutonomousWorldRouteManager::SaveEdge(RouteEdgeInfo const& e)
{
    WorldDatabase.PExecute(
        "REPLACE INTO autonomous_bot_route_edges (from_map, from_cell_x, from_cell_y, to_map, to_cell_x, to_cell_y, traversals, failures, danger, last_used) VALUES ({}, {}, {}, {}, {}, {}, {}, {}, {}, {})",
        e.fromMap, e.fromCellX, e.fromCellY, e.toMap, e.toCellX, e.toCellY,
        e.traversals, e.failures, e.danger, e.lastUsed);
}

void AutonomousWorldRouteManager::ObserveNode(Perception const& p)
{
    if (!_player)
        return;

    uint32 map = _player->GetMapId();
    uint32 x = Cell(_player->GetPositionX());
    uint32 y = Cell(_player->GetPositionY());
    uint64 key = NodeKey(map, x, y);
    LoadNode(map, x, y);

    RouteNodeInfo& n = _nodes[key];
    n.mapId = map; n.cellX = x; n.cellY = y; n.position = p.position; n.position.mapId = map;
    ++n.observations;
    n.lastSeen = static_cast<uint64>(std::time(nullptr));

    uint32 hostile = 0;
    for (WorldObjectInfo const& o : p.nearbyCreatures)
        if (o.hostile && o.alive && o.distance <= 35.0f)
            ++hostile;

    uint32 danger = std::min<uint32>(100, hostile * 15);
    if (_player->IsInCombat())
        danger = std::min<uint32>(100, danger + 25);
    if (p.maxHealth && p.health * 100 < p.maxHealth * 50)
        danger = std::min<uint32>(100, danger + 20);

    n.danger = (n.danger * 3 + danger) / 4;
    if (p.campaignKnowledgeKnown && p.campaignKnowledgeMap == map)
        ++n.objectiveCount;
    _currentDanger = n.danger;

    if (n.observations == 1 || n.observations % 10 == 0 || danger > 60)
        SaveNode(n);

    if (_hadPreviousNode && (_lastMap != map || _lastCellX != x || _lastCellY != y))
        ObserveEdge(map, x, y);

    _lastMap = map; _lastCellX = x; _lastCellY = y; _lastNodeKey = key; _hadPreviousNode = true;
}

void AutonomousWorldRouteManager::ObserveEdge(uint32 map, uint32 x, uint32 y)
{
    uint64 key = EdgeKey(_lastMap, _lastCellX, _lastCellY, map, x, y);
    RouteEdgeInfo& e = _edges[key];
    e.fromMap = _lastMap; e.fromCellX = _lastCellX; e.fromCellY = _lastCellY;
    e.toMap = map; e.toCellX = x; e.toCellY = y;
    ++e.traversals;
    e.danger = _currentDanger;
    _lastEdgeKey = key;
    _hadPreviousEdge = true;
    e.lastUsed = static_cast<uint64>(std::time(nullptr));
    if (e.traversals == 1 || e.traversals % 10 == 0 || e.danger > 60)
        SaveEdge(e);
}

void AutonomousWorldRouteManager::ConsiderNode(RouteNodeInfo const& n, Position const& origin, double& bestScore)
{
    if (n.mapId != origin.mapId)
        return;

    float dx = n.position.x - origin.x, dy = n.position.y - origin.y, dz = n.position.z - origin.z;
    double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (distance < 75.0 || distance > 1200.0)
        return;

    double score = 30.0 + std::min<double>(30.0, n.objectiveCount * 5.0);
    score += std::min<double>(20.0, n.observations / 5.0);
    score -= n.danger * 0.7;
    score -= n.deaths * 8.0;
    score -= distance / 100.0;

    if (_hadPreviousNode)
    {
        uint64 edgeKey = EdgeKey(_lastMap, _lastCellX, _lastCellY, n.mapId, n.cellX, n.cellY);
        auto edge = _edges.find(edgeKey);
        if (edge != _edges.end())
        {
            score += std::min<double>(20.0, edge->second.traversals * 2.0);
            score -= edge->second.failures * 8.0;
            score -= edge->second.danger * 0.3;
        }
    }

    if (score > bestScore)
    {
        bestScore = score;
        _hasPlan = true;
        _planDestination = n.position;
        _planScore = static_cast<uint32>(std::max(0.0, score));
        _routeConfidence = std::min<uint32>(100, n.observations * 5);
    }
}

void AutonomousWorldRouteManager::Plan(Perception const& p)
{
    _hasPlan = false; _needsTaxi = false; _taxiTargetMap = 0; _planScore = 0; _routeConfidence = 0;
    _planStage = "idle"; _planReason = "none";
    _stuck = false;
    _stuckTimer = 0;
    _progressPercent = 0;

    if (!_player || !_player->IsAlive())
        return;
    if (p.inDungeon || p.groupInCombat)
    {
        _planStage = "deferred"; _planReason = "dungeon_or_group_combat"; return;
    }

    Position const origin = p.position;

    if (p.campaignKnowledgeKnown)
    {
        _hasPlan = true;
        _planDestination = p.campaignKnowledgePosition;
        _planDestination.mapId = p.campaignKnowledgeMap;
        _routeConfidence = p.campaignKnowledgeConfidence;
        _planScore = std::min<uint32>(100, p.campaignKnowledgeConfidence + p.campaignKnowledgeDensity * 2);
        _planStage = "objective"; _planReason = "learned_campaign_objective";
        _lastPlanDistance = _player->GetMapId() == _planDestination.mapId ? _player->GetDistance(_planDestination.x, _planDestination.y, _planDestination.z) : -1.0f;
        _bestPlanDistance = _lastPlanDistance;
        _executionState = _planDestination.mapId != origin.mapId ? "taxi" : "traveling";
        if (_planDestination.mapId != origin.mapId)
        {
            _needsTaxi = true; _taxiTargetMap = _planDestination.mapId;
            _planStage = "cross_zone"; _planReason = "learned_cross_zone_objective";
        }
        return;
    }

    // Explicit campaign knowledge and visible quest givers must win over
    // opportunistic route-memory exploration. Otherwise a well-known old
    // route node can pull a questing bot away from an immediately actionable
    // quest.
    double nearest = std::numeric_limits<double>::max();
    for (QuestInfo const& q : p.quests)
    {
        if (!q.giverGuid || (!q.canAccept && !q.canComplete) || q.giverDistance >= nearest)
            continue;

        for (WorldObjectInfo const& o : p.nearbyCreatures)
            if (o.guid == q.giverGuid)
            {
                nearest = q.giverDistance;
                _hasPlan = true;
                _planDestination = o.position;
                _planStage = "quest_giver";
                _planReason = q.canComplete ? "visible_quest_turnin" : "visible_quest";
                _planScore = q.canComplete ? 90 : 70;
                _lastPlanDistance = q.giverDistance;
                _bestPlanDistance = q.giverDistance;
                _executionState = "traveling";
                return;
            }
    }

    double bestScore = -std::numeric_limits<double>::infinity();
    for (auto const& pair : _nodes)
        ConsiderNode(pair.second, origin, bestScore);

    if (_hasPlan)
    {
        _planStage = "learned_route"; _planReason = "population_route_memory";
        _lastPlanDistance = _player->GetMapId() == _planDestination.mapId ? _player->GetDistance(_planDestination.x, _planDestination.y, _planDestination.z) : -1.0f;
        _bestPlanDistance = _lastPlanDistance;
        _executionState = "traveling";
        return;
    }

}

void AutonomousWorldRouteManager::Update(uint32 diff, Perception const& p)
{
    if (!_player || !_player->IsInWorld())
        return;

    // Deaths are sampled on the update timer; record a death only once per corpse state.
    if (_player->IsAlive())
        _deathRecorded = false;

    // Deaths are sampled on the update timer; record a death only once per corpse state.
    if (_player->IsAlive())
        _deathRecorded = false;

    if (_timer > diff)
        _timer -= diff;
    else
    {
        _timer = 3000;
        if (_player->IsAlive())
        {
            ObserveNode(p);
            LoadMapNodes(_player->GetMapId());
            LoadEdges(_player->GetMapId(), Cell(_player->GetPositionX()), Cell(_player->GetPositionY()));
        }
        else if (_hadPreviousNode && !_deathRecorded)
        {
            _deathRecorded = true;
            auto itr = _nodes.find(_lastNodeKey);
            if (itr != _nodes.end())
            {
                ++itr->second.deaths;
                itr->second.danger = std::min<uint32>(100, itr->second.danger + 20);
                SaveNode(itr->second);
            }
            if (_hadPreviousEdge)
            {
                auto edge = _edges.find(_lastEdgeKey);
                if (edge != _edges.end())
                {
                    ++edge->second.failures;
                    edge->second.danger = std::min<uint32>(100, edge->second.danger + 20);
                    SaveEdge(edge->second);
                }
                _hadPreviousEdge = false;
            }
        }
    }

    if (_hasPlan && _player->IsAlive())
    {
        if (_needsTaxi || _planDestination.mapId != _player->GetMapId())
        {
            _executionState = "taxi";
            _stuckTimer = 0;
            _stuck = false;
            _progressPercent = 0;
        }
        else
        {
            float distance = _player->GetDistance(_planDestination.x, _planDestination.y, _planDestination.z);
            if (distance <= 8.0f)
            {
                _executionState = "arrived";
                _stuckTimer = 0;
                _stuck = false;
                _progressPercent = 100;
            }
            else
            {
                if (_lastPlanDistance < 0.0f)
                {
                    _lastPlanDistance = distance;
                    _bestPlanDistance = distance;
                }

                if (distance + 1.0f < _bestPlanDistance)
                {
                    _bestPlanDistance = distance;
                    _stuckTimer = 0;
                    _stuck = false;
                }
                else if (distance >= _lastPlanDistance - 0.5f)
                    _stuckTimer += diff;
                else
                    _stuckTimer = 0;

                _lastPlanDistance = distance;

                if (_bestPlanDistance > 8.0f)
                {
                    float progress = (_bestPlanDistance - distance) / _bestPlanDistance;
                    _progressPercent = static_cast<uint32>(std::clamp(progress * 100.0f, 0.0f, 100.0f));
                }

                if (_stuckTimer >= 15000)
                {
                    _stuck = true;
                    _executionState = "stuck";
                    ++_replanCount;
                    if (_hadPreviousEdge)
                    {
                        auto edge = _edges.find(_lastEdgeKey);
                        if (edge != _edges.end())
                        {
                            ++edge->second.failures;
                            edge->second.danger = std::min<uint32>(100, edge->second.danger + 10);
                            SaveEdge(edge->second);
                        }
                        _hadPreviousEdge = false;
                    }
                    _hasPlan = false;
                    _planTimer = 0;
                    _stuckTimer = 0;
                }
                else
                    _executionState = "traveling";
            }
        }
    }
    else if (!_hasPlan)
    {
        _executionState = "idle";
        _progressPercent = 0;
    }

    if (_planTimer > diff)
        _planTimer -= diff;
    else
    {
        _planTimer = 5000;
        Plan(p);
    }
}
}
