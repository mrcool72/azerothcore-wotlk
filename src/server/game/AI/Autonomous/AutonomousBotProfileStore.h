#ifndef TRINITY_AUTONOMOUS_BOT_PROFILE_STORE_H
#define TRINITY_AUTONOMOUS_BOT_PROFILE_STORE_H

#include "AutonomousBotProfile.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace AutonomousAI
{
    class AutonomousBotProfileStore
    {
    public:
        void Load();
        bool Save(AutonomousBotProfile const& profile);
        bool Remove(uint64 guid);
        bool Get(uint64 guid, AutonomousBotProfile& profile) const;
        std::vector<AutonomousBotProfile> GetAll() const;

    private:
        std::unordered_map<uint64, AutonomousBotProfile> _profiles;
    };

    extern AutonomousBotProfileStore sAutonomousBotProfileStore;
}

#endif
