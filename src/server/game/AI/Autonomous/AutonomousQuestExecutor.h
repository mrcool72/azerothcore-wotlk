#ifndef TRINITY_AUTONOMOUS_QUEST_EXECUTOR_H
#define TRINITY_AUTONOMOUS_QUEST_EXECUTOR_H

#include "AutonomousBotProtocol.h"

#include <cstdint>

class Player;

namespace AutonomousAI
{
    class AutonomousQuestExecutor
    {
    public:
        explicit AutonomousQuestExecutor(Player* player);

        void Update(uint32 diff, Perception const& perception);
        bool Start(uint32 questId);
        void Cancel();

        bool IsActive() const { return _questId != 0; }
        uint32 GetQuestId() const { return _questId; }
        char const* GetStateName() const;

    private:
        enum class State : uint8
        {
            IDLE,
            FIND_GIVER,
            TRAVEL_TO_GIVER,
            ACCEPT,
            EXECUTE,
            RETURN_TO_GIVER,
            TURN_IN,
            DONE
        };

        bool FindGiver(Perception const& perception, uint64& guid, Position& position, bool involved) const;
        bool Interact(uint64 guid, bool reward);
        void MoveTo(Position const& position);

        Player* _player;
        uint32 _questId;
        State _state;
        uint32 _thinkTimer;
        uint64 _giverGuid;
    };
}

#endif
