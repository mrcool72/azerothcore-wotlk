#ifndef TRINITY_AUTONOMOUS_RELATIONSHIP_MANAGER_H
#define TRINITY_AUTONOMOUS_RELATIONSHIP_MANAGER_H

#include "Common.h"
#include <string>
#include <unordered_map>

namespace AutonomousAI
{
    struct RelationshipMemory
    {
        int32 score = 0;
        uint32 interactions = 0;
        bool preferred = false;
    };

    class AutonomousRelationshipManager
    {
    public:
        void RecordInteraction(uint64 botGuid, uint64 otherGuid, int32 delta = 1);
        RelationshipMemory Get(uint64 botGuid, uint64 otherGuid);
        int32 GetScore(uint64 botGuid, uint64 otherGuid);
        uint64 GetPreferredCompanion(uint64 botGuid);
        int32 GetSocialScore(uint64 botGuid);
        uint32 GetInteractionCount(uint64 botGuid);
        void SetPreferred(uint64 botGuid, uint64 otherGuid, bool preferred);

    private:
        static uint64 MakeKey(uint64 botGuid, uint64 otherGuid);
        RelationshipMemory Load(uint64 botGuid, uint64 otherGuid);
        void Save(uint64 botGuid, uint64 otherGuid, RelationshipMemory const& memory);

        struct Summary
        {
            uint64 preferredGuid = 0;
            int32 score = 0;
            uint32 interactions = 0;
            bool loaded = false;
        };

        std::unordered_map<uint64, RelationshipMemory> _cache;
        std::unordered_map<uint64, Summary> _summaries;
    };

    extern AutonomousRelationshipManager sAutonomousRelationshipMgr;
}

#endif
