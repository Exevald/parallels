#pragma once

#include "AudioVisualizer.h"
#include "Cli.h"
#include "ConcurrentQueue.h"
#include "Protocol.h"
#include "TcpChannel.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace radio
{
class Station
{
public:
    Station(Options options, std::atomic_bool& stopRequested);
    ~Station();

    int Run();

private:
    using SocketPtr = std::shared_ptr<TcpSocket>;
    using Acceptor = boost::asio::ip::tcp::acceptor;

    void AcceptLoop();
    void AudioSendLoop();
    void Shutdown();
    void BroadcastAudio(const AudioChunk& chunk);
    void SendDisconnectToClients();
    void RemoveDeadSockets(std::vector<SocketPtr>& sockets, const std::vector<size_t>& deadIndices);

    const Options m_options;
    std::atomic_bool& m_stopRequested;
    StreamConfig m_streamConfig;

    boost::asio::io_context m_ioContext;
    std::unique_ptr<Acceptor> m_listener;

    std::mutex m_clientsMutex;
    std::vector<SocketPtr> m_clients;

    BoundedQueue<AudioChunk> m_audioQueue{128};
    std::thread m_acceptThread;
    std::thread m_audioSendThread;
    uint64_t m_audioSequence = 0;
    bool m_shutdownStarted = false;

    AudioVisualizer m_visualizer;
};
}
