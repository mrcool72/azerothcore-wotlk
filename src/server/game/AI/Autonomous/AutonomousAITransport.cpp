#include "AutonomousAcoreCompat.h"
/*
 * External AI transport implementation.
 */

#include "AutonomousAITransport.h"

#include "Log.h"

#include <boost/asio.hpp>

#include <chrono>
#include <sstream>

namespace AutonomousAI
{
    ExternalAITransport::ExternalAITransport() :
        _host("127.0.0.1"),
        _port(8765),
        _running(false),
        _worker(),
        _queueMutex(),
        _queueCondition(),
        _requests(),
        _responses()
    {
    }

    ExternalAITransport::~ExternalAITransport()
    {
        Stop();
    }

    void ExternalAITransport::Configure(std::string host, uint16 port)
    {
        std::lock_guard<std::mutex> lock(_queueMutex);
        _host = std::move(host);
        _port = port;
    }

    bool ExternalAITransport::Start()
    {
        bool expected = false;
        if (!_running.compare_exchange_strong(expected, true))
            return false;

        _worker = std::thread(&ExternalAITransport::WorkerLoop, this);
        return true;
    }

    void ExternalAITransport::Stop()
    {
        if (!_running.exchange(false))
            return;

        _queueCondition.notify_all();

        if (_worker.joinable())
            _worker.join();

        std::lock_guard<std::mutex> lock(_queueMutex);
        _requests.clear();
        _responses.clear();
    }

    void ExternalAITransport::Submit(uint64 botGuid, std::string message)
    {
        if (!_running.load())
            return;

        {
            std::lock_guard<std::mutex> lock(_queueMutex);

            // Keep only a small amount of stale perception for each bot.
            while (_requests.size() > 32)
                _requests.pop_front();

            _requests.push_back({ botGuid, std::move(message) });
        }

        _queueCondition.notify_one();
    }

    bool ExternalAITransport::TryPopAction(uint64 botGuid, Action& action)
    {
        std::lock_guard<std::mutex> lock(_queueMutex);

        for (auto itr = _responses.begin(); itr != _responses.end(); ++itr)
        {
            if (itr->botGuid != botGuid)
                continue;

            Response response = std::move(*itr);
            _responses.erase(itr);
            return ParseAction(response.message, action);
        }

        return false;
    }

    bool ExternalAITransport::Exchange(Request const& request, std::string& response)
    {
        try
        {
            boost::asio::io_context ioContext;
            boost::asio::ip::tcp::resolver resolver(ioContext);
            boost::asio::ip::tcp::socket socket(ioContext);

            boost::system::error_code error;
            auto endpoints = resolver.resolve(_host, std::to_string(_port), error);
            if (error)
                return false;

            boost::asio::connect(socket, endpoints, error);
            if (error)
                return false;

            std::string payload = request.message;
            if (payload.empty() || payload.back() != '\n')
                payload.push_back('\n');

            boost::asio::write(socket, boost::asio::buffer(payload), error);
            if (error)
                return false;

            boost::asio::streambuf buffer;
            boost::asio::read_until(socket, buffer, '\n', error);
            if (error)
                return false;

            std::istream stream(&buffer);
            std::getline(stream, response);
            return !response.empty();
        }
        catch (...)
        {
            return false;
        }
    }

    void ExternalAITransport::WorkerLoop()
    {
        TC_LOG_INFO("server.loading", "Autonomous AI external transport started on {}:{}", _host, _port);

        while (_running.load())
        {
            Request request;

            {
                std::unique_lock<std::mutex> lock(_queueMutex);
                _queueCondition.wait_for(lock, std::chrono::milliseconds(250), [this]()
                {
                    return !_running.load() || !_requests.empty();
                });

                if (!_running.load())
                    break;

                if (_requests.empty())
                    continue;

                request = std::move(_requests.front());
                _requests.pop_front();
            }

            std::string response;
            if (!Exchange(request, response))
            {
                TC_LOG_DEBUG("server.loading", "Autonomous AI server unavailable at {}:{}", _host, _port);
                continue;
            }

            {
                std::lock_guard<std::mutex> lock(_queueMutex);
                if (_responses.size() > 64)
                    _responses.pop_front();

                _responses.push_back({ request.botGuid, std::move(response) });
            }
        }

        TC_LOG_INFO("server.loading", "Autonomous AI external transport stopped");
    }
}
