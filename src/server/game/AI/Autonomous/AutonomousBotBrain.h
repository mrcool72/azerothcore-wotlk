#ifndef TRINITY_AUTONOMOUS_BOT_BRAIN_H
#define TRINITY_AUTONOMOUS_BOT_BRAIN_H

#include "AutonomousBotProtocol.h"

#include <string>
#include <utility>

class Player;

namespace AutonomousAI
{
    class AutonomousBotBrain
    {
    public:
        explicit AutonomousBotBrain(Player* player);

        void Update(uint32 diff, Perception const& perception, bool questExecutorActive);
        void SetEnabled(bool enabled) { _enabled = enabled; }
        void SetPersonality(std::string personality) { _personality = std::move(personality); }
        std::string const& GetPersonality() const { return _personality; }
        bool IsEnabled() const { return _enabled; }
        uint32 ConsumeQuestRequest();

    private:
        bool SelectQuest(Perception const& perception);
        bool WorkOnQuest(Perception const& perception);
        bool Recover();
        void Explore();
        void MoveTo(Position const& position);

        Player* _player;
        bool _enabled;
        uint32 _thinkTimer;
        uint32 _exploreTimer;
        uint32 _questRequest;
        std::string _personality;
        bool _recovering;
    };
}

#endif
