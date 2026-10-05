#pragma once

#include "Define.h"
#include "AutonomousBotProtocol.h"

class Player;

namespace AutonomousAI
{

    class AutonomousIntegrationManager
    {
    public:
        void Update(uint32 diff, Player* player, Perception const& perception, bool externalActive);

        bool HasLocalAction() const { return _hasAction; }
        Action const& GetAction() const { return _action; }
        std::string const& GetDecision() const { return _decision; }
        uint32 GetDecisionCount() const { return _decisionCount; }

    private:
        uint32 _timer = 0;
        bool _hasAction = false;
        Action _action;
        std::string _decision = "idle";
        uint32 _decisionCount = 0;
    };
}
