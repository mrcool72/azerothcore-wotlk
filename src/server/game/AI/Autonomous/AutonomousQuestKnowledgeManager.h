#ifndef TRINITY_AUTONOMOUS_QUEST_KNOWLEDGE_MANAGER_H
#define TRINITY_AUTONOMOUS_QUEST_KNOWLEDGE_MANAGER_H

#include "AutonomousBotProtocol.h"
#include <unordered_map>
#include <vector>

namespace AutonomousAI
{
struct QuestWorldKnowledge
{
    uint32 questId = 0;
    uint32 entry = 0;
    uint32 mapId = 0;
    Position position;
    uint32 observations = 0;
    uint64 lastSeen = 0;
    uint32 confidence = 0;
    uint32 density = 0;
    uint32 cellX = 0;
    uint32 cellY = 0;
};

class AutonomousQuestKnowledgeManager
{
public:
    void Update(uint32 diff, Perception const& perception);
    bool Find(uint32 questId, uint32 entry, QuestWorldKnowledge& result) const;
    bool FindBest(uint32 questId, uint32 entry, uint32 preferredMapId, Position const* origin, QuestWorldKnowledge& result) const;
    uint32 GetKnownCount() const { return static_cast<uint32>(_cache.size()); }
    uint32 GetLocationCount(uint32 questId, uint32 entry) const;

private:
    static uint64 MakeKey(uint32 questId, uint32 entry, uint32 mapId, uint32 cellX = 0, uint32 cellY = 0);
    static uint32 Cell(float coordinate);
    void LoadQuest(uint32 questId, uint32 entry, uint32 mapId);
    void LoadLocations(uint32 questId, uint32 entry);
    void Save(QuestWorldKnowledge const& knowledge);

    uint32 _timer = 0;
    std::unordered_map<uint64, QuestWorldKnowledge> _cache;
};

extern AutonomousQuestKnowledgeManager sAutonomousQuestKnowledgeMgr;
}

#endif
