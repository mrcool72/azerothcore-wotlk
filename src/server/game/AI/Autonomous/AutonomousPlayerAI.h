#ifndef AC_AUTONOMOUS_PLAYER_AI_H
#define AC_AUTONOMOUS_PLAYER_AI_H

#include "UnitAI.h"
#include "AutonomousBotProtocol.h"

class Player;

namespace AutonomousAI
{
    class AutonomousBotController;

    class AutonomousPlayerAI final : public UnitAI
    {
    public:
        explicit AutonomousPlayerAI(Player* player);
        ~AutonomousPlayerAI() override = default;

        void UpdateAI(uint32 diff) override;
        void OnCharmed(bool /*apply*/) override { }

        AutonomousBotController* GetController() const { return _controller; }
        void SetController(AutonomousBotController* controller) { _controller = controller; }

        GoalType GetGoal() const { return _goal; }
        void SetGoal(GoalType goal) { _goal = goal; }

    private:
        Player* _player;
        AutonomousBotController* _controller;
        GoalType _goal;
        uint32 _thinkTimer;
    };
}

#endif
