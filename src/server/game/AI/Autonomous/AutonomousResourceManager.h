#ifndef TRINITY_AUTONOMOUS_RESOURCE_MANAGER_H
#define TRINITY_AUTONOMOUS_RESOURCE_MANAGER_H

#include "Common.h"
#include <string>

namespace AutonomousAI
{
    struct Perception;

    class AutonomousResourceManager
    {
    public:
        void Update(uint32 diff, Perception const& perception);
        bool HasCandidate() const { return _candidateGuid != 0; }
        uint64 GetCandidateGuid() const { return _candidateGuid; }
        float GetCandidateDistance() const { return _candidateDistance; }
        std::string const& GetCandidateName() const { return _candidateName; }
        uint32 GetCandidateCount() const { return _candidateCount; }

    private:
        uint32 _timer = 0;
        uint64 _candidateGuid = 0;
        float _candidateDistance = 0.0f;
        std::string _candidateName;
        uint32 _candidateCount = 0;
    };
}

#endif
