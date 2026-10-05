#ifndef TRINITY_AUTONOMOUS_BOT_PROFILE_H
#define TRINITY_AUTONOMOUS_BOT_PROFILE_H

#include "Common.h"
#include <string>

namespace AutonomousAI
{
    struct AutonomousBotProfile
    {
        uint64 guid = 0;
        std::string name;
        std::string host = "127.0.0.1";
        uint16 port = 8765;
        bool enabled = false;
        std::string personality = "balanced";
        std::string role = "auto";
    };
}

#endif
