#ifndef TRINITY_AUTONOMOUS_WORLD_ROUTE_MANAGER_H
#define TRINITY_AUTONOMOUS_WORLD_ROUTE_MANAGER_H

#include "AutonomousBotProtocol.h"
#include <unordered_map>

class Player;

namespace AutonomousAI
{
struct RouteNodeInfo
{
    uint32 mapId = 0;
    uint32 cellX = 0;
    uint32 cellY = 0;
    Position position;
    uint32 observations = 0;
    uint32 deaths = 0;
    uint32 danger = 0;
    uint32 objectiveCount = 0;
    uint64 lastSeen = 0;
};

struct RouteEdgeInfo
{
    uint32 fromMap = 0;
    uint32 fromCellX = 0;
    uint32 fromCellY = 0;
    uint32 toMap = 0;
    uint32 toCellX = 0;
    uint32 toCellY = 0;
    uint32 traversals = 0;
    uint32 failures = 0;
    uint32 danger = 0;
    uint64 lastUsed = 0;
};

class AutonomousWorldRouteManager
{
public:
    explicit AutonomousWorldRouteManager(Player* player);
    void Update(uint32 diff, Perception const& perception);

    bool HasPlan() const { return _hasPlan; }
    Position const& GetPlanDestination() const { return _planDestination; }
    char const* GetPlanStage() const { return _planStage.c_str(); }
    char const* GetPlanReason() const { return _planReason.c_str(); }
    uint32 GetPlanScore() const { return _planScore; }
    uint32 GetCurrentDanger() const { return _currentDanger; }
    uint32 GetKnownNodes() const { return static_cast<uint32>(_nodes.size()); }
    uint32 GetKnownEdges() const { return static_cast<uint32>(_edges.size()); }
    uint32 GetRouteConfidence() const { return _routeConfidence; }
    bool NeedsTaxi() const { return _needsTaxi; }
    uint32 GetTaxiTargetMap() const { return _taxiTargetMap; }
    char const* GetExecutionState() const { return _executionState.c_str(); }
    bool IsStuck() const { return _stuck; }
    uint32 GetReplanCount() const { return _replanCount; }
    uint32 GetProgressPercent() const { return _progressPercent; }

    static uint32 Cell(float coordinate);
    static uint64 NodeKey(uint32 mapId, uint32 cellX, uint32 cellY);
    static uint64 EdgeKey(uint32 fromMap, uint32 fromCellX, uint32 fromCellY,
        uint32 toMap, uint32 toCellX, uint32 toCellY);

private:
    void ObserveNode(Perception const& perception);
    void ObserveEdge(uint32 nowMap, uint32 nowCellX, uint32 nowCellY);
    void Plan(Perception const& perception);
    void LoadNode(uint32 mapId, uint32 cellX, uint32 cellY);
    void LoadMapNodes(uint32 mapId);
    void LoadEdges(uint32 mapId, uint32 cellX, uint32 cellY);
    void SaveNode(RouteNodeInfo const& node);
    void SaveEdge(RouteEdgeInfo const& edge);
    void ConsiderNode(RouteNodeInfo const& node, Position const& origin, double& bestScore);

    Player* _player;
    uint32 _timer = 0;
    uint32 _planTimer = 0;
    uint32 _currentDanger = 0;
    uint32 _routeConfidence = 0;
    bool _hasPlan = false;
    bool _needsTaxi = false;
    uint32 _taxiTargetMap = 0;
    uint32 _planScore = 0;
    Position _planDestination;
    std::string _planStage = "idle";
    std::string _planReason = "none";
    uint32 _lastMap = 0;
    uint32 _lastCellX = 0;
    uint32 _lastCellY = 0;
    uint64 _lastNodeKey = 0;
    bool _hadPreviousNode = false;
    uint64 _lastEdgeKey = 0;
    bool _hadPreviousEdge = false;
    bool _deathRecorded = false;
    bool _stuck = false;
    uint32 _stuckTimer = 0;
    uint32 _replanCount = 0;
    uint32 _progressPercent = 0;
    float _lastPlanDistance = -1.0f;
    float _bestPlanDistance = -1.0f;
    std::string _executionState = "idle";
    std::unordered_map<uint64, RouteNodeInfo> _nodes;
    std::unordered_map<uint64, RouteEdgeInfo> _edges;
};
}
#endif
