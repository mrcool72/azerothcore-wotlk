#include "AutonomousPlayerAI.h"

#include "AutonomousBotController.h"
#include "Player.h"

namespace AutonomousAI
{
    AutonomousPlayerAI::AutonomousPlayerAI(Player* player) :
        UnitAI(player),
        _player(player),
        _controller(nullptr),
        _goal(GoalType::NONE),
        _thinkTimer(1000)
    {
    }

    void AutonomousPlayerAI::UpdateAI(uint32 diff)
    {
        if (!_controller || !_player || !_player->IsInWorld())
            return;

        if (_thinkTimer > diff)
        {
            _thinkTimer -= diff;
            return;
        }

        _thinkTimer = 1000;
        _controller->Update(diff);
    }
}
