/*
 * Autonomous bot controller.
 */

#ifndef TRINITY_AUTONOMOUS_BOT_CONTROLLER_H
#define TRINITY_AUTONOMOUS_BOT_CONTROLLER_H

#include "AutonomousBotProtocol.h"

#include <deque>
#include <memory>

class Player;

namespace AutonomousAI
{
    class AutonomousPlayerAI;
    class ExternalAITransport;
    class AutonomousQuestExecutor;
    class AutonomousBotBrain;
    class AutonomousCombatManager;
    class AutonomousDungeonManager;
    class AutonomousWorldManager;
    class AutonomousTravelManager;
    class AutonomousTownManager;
    class AutonomousEquipmentManager;
    class AutonomousMemoryManager;
    class AutonomousNavigationManager;
    class AutonomousResourceManager;
    class AutonomousEconomyManager;
    class AutonomousLifeManager;
    class AutonomousIntegrationManager;
    class AutonomousTaxiManager;
    class AutonomousServiceManager;
    class AutonomousQuestCampaignManager;
    class AutonomousCampaignExecutor;
    class AutonomousCampaignNavigationManager;
    class AutonomousQuestKnowledgeManager;
    class AutonomousWorldRouteManager;

    class AutonomousBotController
    {
    public:
        explicit AutonomousBotController(Player* player);
        ~AutonomousBotController();

        Player* GetPlayer() const { return _player; }
        AutonomousPlayerAI* GetAI() const { return _ai; }
        void SetAI(AutonomousPlayerAI* ai) { _ai = ai; }

        void Update(uint32 diff);

        void PushAction(Action const& action);
        bool PopAction(Action& action);
        bool HasPendingActions() const { return !_actions.empty(); }

        Perception const& GetPerception() const { return _perception; }
        void RefreshPerception();

        void EnableExternalAI(bool enabled) { _externalAIEnabled = enabled; }
        bool IsExternalAIEnabled() const { return _externalAIEnabled; }
        void SetPersonality(std::string personality);
        void SetRole(std::string role);

        bool StartQuest(uint32 questId);
        void CancelQuest();
        uint32 GetActiveQuest() const;
        char const* GetQuestState() const;
        uint64 GetPreferredCompanionGuid() const;
    char const* GetRole() const;

        void ConfigureExternalAI(std::string host, uint16 port, std::string token = {});
        void SetExternalAIRequestInterval(uint32 milliseconds);
        bool StartExternalAI();
        void StopExternalAI();

    private:
        void BuildPerception();
        void ExecuteAction(Action const& action);
        void SubmitPerception();

        Player* _player;
        AutonomousPlayerAI* _ai;
        std::deque<Action> _actions;
        Perception _perception;
        std::unique_ptr<ExternalAITransport> _transport;
        std::unique_ptr<AutonomousQuestExecutor> _questExecutor;
        std::unique_ptr<AutonomousBotBrain> _brain;
        std::unique_ptr<AutonomousCombatManager> _combatManager;
        std::unique_ptr<AutonomousDungeonManager> _dungeonManager;
        std::unique_ptr<AutonomousWorldManager> _worldManager;
        std::unique_ptr<AutonomousTravelManager> _travelManager;
        std::unique_ptr<AutonomousTownManager> _townManager;
        std::unique_ptr<AutonomousEquipmentManager> _equipmentManager;
        std::unique_ptr<AutonomousMemoryManager> _memoryManager;
        std::unique_ptr<AutonomousNavigationManager> _navigationManager;
        std::unique_ptr<AutonomousResourceManager> _resourceManager;
        std::unique_ptr<AutonomousEconomyManager> _economyManager;
        std::unique_ptr<AutonomousLifeManager> _lifeManager;
        std::unique_ptr<AutonomousIntegrationManager> _integrationManager;
        std::unique_ptr<AutonomousTaxiManager> _taxiManager;
        std::unique_ptr<AutonomousServiceManager> _serviceManager;
        std::unique_ptr<AutonomousQuestCampaignManager> _campaignManager;
        std::unique_ptr<AutonomousCampaignExecutor> _campaignExecutor;
        std::unique_ptr<AutonomousCampaignNavigationManager> _campaignNavigationManager;
        AutonomousQuestKnowledgeManager* _questKnowledgeManager;
        std::unique_ptr<AutonomousWorldRouteManager> _worldRouteManager;
        bool _externalAIEnabled;
        uint32 _requestTimer;
    };
}

#endif
