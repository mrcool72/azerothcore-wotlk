#include "AutonomousResourceManager.h"
#include "AutonomousBotProtocol.h"

namespace AutonomousAI
{
    void AutonomousResourceManager::Update(uint32 diff, Perception const& perception)
    {
        if (_timer > diff)
        {
            _timer -= diff;
            return;
        }
        _timer = 3000;

        _candidateGuid = 0;
        _candidateDistance = 0.0f;
        _candidateName.clear();
        _candidateCount = 0;

        // TrinityCore remains authoritative for the actual interaction.  This
        // manager deliberately exposes only nearby world-object opportunities.
        // Quest-linked objects are already identified by perception and are the
        // safest resource/objective candidates for autonomous planning.
        for (WorldObjectInfo const& object : perception.nearbyGameObjects)
        {
            if (!object.guid || object.distance > 60.0f || !object.alive)
                continue;

            if (!object.questObjectiveItem && object.questGiver)
                continue;

            ++_candidateCount;
            if (!_candidateGuid || object.distance < _candidateDistance)
            {
                _candidateGuid = object.guid;
                _candidateDistance = object.distance;
                _candidateName = object.name;
            }
        }
    }
}
