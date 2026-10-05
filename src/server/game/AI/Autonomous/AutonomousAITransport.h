/*
 * External AI transport.
 *
 * Protocol: one JSON document per line over TCP.
 * TrinityCore sends perception messages; the AI server replies with one action JSON object.
 */

#ifndef TRINITY_AUTONOMOUS_AI_TRANSPORT_H
#define TRINITY_AUTONOMOUS_AI_TRANSPORT_H

#include "AutonomousBotProtocol.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

namespace AutonomousAI
{
    class ExternalAITransport
    {
    public:
        ExternalAITransport();
        ~ExternalAITransport();

        ExternalAITransport(ExternalAITransport const&) = delete;
        ExternalAITransport& operator=(ExternalAITransport const&) = delete;

        void Configure(std::string host, uint16 port);
        bool Start();
        void Stop();

        bool IsRunning() const { return _running.load(); }

        // Called from the world thread. This only queues work; it never performs network I/O.
        void Submit(uint64 botGuid, std::string message);

        // Called from the world thread. Returns the oldest action for this bot.
        bool TryPopAction(uint64 botGuid, Action& action);

    private:
        struct Request
        {
            uint64 botGuid = 0;
            std::string message;
        };

        struct Response
        {
            uint64 botGuid = 0;
            std::string message;
        };

        void WorkerLoop();
        bool Exchange(Request const& request, std::string& response);

        std::string _host;
        uint16 _port;

        std::atomic<bool> _running;
        std::thread _worker;

        std::mutex _queueMutex;
        std::condition_variable _queueCondition;
        std::deque<Request> _requests;
        std::deque<Response> _responses;
    };
}

#endif
