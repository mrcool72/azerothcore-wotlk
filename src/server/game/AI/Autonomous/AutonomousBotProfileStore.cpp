#include "AutonomousAcoreCompat.h"
#include "AutonomousBotProfileStore.h"
#include "DatabaseEnv.h"
#include "Log.h"

#include <algorithm>
#include <cctype>

namespace AutonomousAI
{
    AutonomousBotProfileStore sAutonomousBotProfileStore;

    namespace
    {
        std::string Sanitize(std::string value)
        {
            std::string out;
            out.reserve(value.size());
            for (char c : value)
                if (std::isalnum(static_cast<unsigned char>(c)) || c == '.' || c == ':' || c == '-' || c == '_' || c == ' ')
                    out.push_back(c);
            return out;
        }
    }

    void AutonomousBotProfileStore::Load()
    {
        _profiles.clear();
        if (QueryResult result = WorldDatabase.Query("SELECT guid, name, host, port, enabled, personality, role FROM autonomous_bot_config"))
        {
            do
            {
                Field* fields = result->Fetch();
                AutonomousBotProfile profile;
                profile.guid = fields[0].GetUInt64();
                profile.name = fields[1].GetString();
                profile.host = fields[2].GetString();
                profile.port = fields[3].GetUInt16();
                profile.enabled = fields[4].GetUInt8() != 0;
                profile.personality = fields[5].GetString();
                profile.role = fields[6].GetString();
                _profiles[profile.guid] = std::move(profile);
            } while (result->NextRow());
        }
        LOG_INFO("server.loading", "Loaded {} autonomous bot profiles.", _profiles.size());
    }

    bool AutonomousBotProfileStore::Save(AutonomousBotProfile const& profile)
    {
        std::string name = Sanitize(profile.name);
        std::string host = Sanitize(profile.host);
        std::string personality = Sanitize(profile.personality);
        std::string role = Sanitize(profile.role);
        if (host.empty() || personality.empty() || role.empty())
            return false;

        WorldDatabase.Execute("REPLACE INTO autonomous_bot_config (guid, name, host, port, enabled, personality, role) VALUES ({}, '{}', '{}', {}, {}, '{}', '{}')",
            profile.guid, name, host, profile.port, profile.enabled ? 1 : 0, personality, role);
        _profiles[profile.guid] = profile;
        return true;
    }

    bool AutonomousBotProfileStore::Remove(uint64 guid)
    {
        WorldDatabase.PExecute("DELETE FROM autonomous_bot_config WHERE guid = {}", guid);
        return _profiles.erase(guid) != 0;
    }

    bool AutonomousBotProfileStore::Get(uint64 guid, AutonomousBotProfile& profile) const
    {
        auto itr = _profiles.find(guid);
        if (itr == _profiles.end())
            return false;
        profile = itr->second;
        return true;
    }

    std::vector<AutonomousBotProfile> AutonomousBotProfileStore::GetAll() const
    {
        std::vector<AutonomousBotProfile> result;
        result.reserve(_profiles.size());
        for (auto const& [guid, profile] : _profiles)
            result.push_back(profile);
        std::sort(result.begin(), result.end(), [](auto const& a, auto const& b) { return a.name < b.name; });
        return result;
    }
}
