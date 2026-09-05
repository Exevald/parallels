#include "Station.h"

#include "AudioRecorder.h"

#include <SFML/Audio/SoundRecorder.hpp>

#include <boost/asio/ip/address.hpp>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

namespace radio
{
Station::Station(Options options, std::atomic_bool& stopRequested)
    : m_options(std::move(options))
    , m_stopRequested(stopRequested)
    , m_visualizer("radio station", m_streamConfig.audioChannels)
{
}

Station::~Station()
{
    Shutdown();
}

int Station::Run()
{
    if (!sf::SoundRecorder::isAvailable())
    {
        std::cerr << "Audio capture is not available on this machine\n";
        return EXIT_FAILURE;
    }

    try
    {
        using boost::asio::ip::address_v4;
        using boost::asio::ip::tcp;
        m_listener = std::make_unique<Acceptor>(
            m_ioContext,
            tcp::endpoint(address_v4::any(), m_options.port));
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Failed to bind station listener: " << exception.what() << "\n";
        return EXIT_FAILURE;
    }

    m_acceptThread = std::thread(&Station::AcceptLoop, this);
    m_audioSendThread = std::thread(&Station::AudioSendLoop, this);

    NetworkAudioRecorder recorder([this](AudioChunk chunk) {
        m_visualizer.UpdateSamples(chunk.samples);
        m_audioQueue.PushDropOldest(std::move(chunk));
    });
    recorder.setChannelCount(m_streamConfig.audioChannels);
    if (!recorder.start(m_streamConfig.audioSampleRate))
    {
        std::cerr << "Failed to start microphone capture\n";
        m_stopRequested = true;
        Shutdown();
        return EXIT_FAILURE;
    }

    std::cout << "radio station started\n";
    std::cout << "port: " << m_options.port << "\n";
    std::cout << "sample rate: " << m_streamConfig.audioSampleRate << "\n";
    std::cout << "channels: " << m_streamConfig.audioChannels << "\n";

    m_visualizer.Run(m_stopRequested);

    recorder.stop();
    Shutdown();
    return EXIT_SUCCESS;
}

void Station::AcceptLoop()
{
    while (!m_stopRequested.load())
    {
        auto socket = std::make_shared<TcpSocket>(m_ioContext);
        boost::system::error_code errorCode;
        m_listener->accept(*socket, errorCode);
        if (errorCode)
        {
            if (!m_stopRequested.load())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
            continue;
        }

        const std::vector<std::byte> payload = SerializeStreamConfig(m_streamConfig);
        const PacketHeader header{
            .type = PacketType::StreamConfig,
            .payloadSize = static_cast<uint32_t>(payload.size()),
        };
        if (!SendPacket(*socket, header, payload))
        {
            boost::system::error_code closeError;
            socket->close(closeError);
            continue;
        }

        std::lock_guard lock(m_clientsMutex);
        m_clients.push_back(std::move(socket));
    }
}

void Station::AudioSendLoop()
{
    while (!m_stopRequested.load())
    {
        const auto chunk = m_audioQueue.WaitPop();
        if (!chunk.has_value())
        {
            break;
        }

        BroadcastAudio(*chunk);
    }
}

void Station::Shutdown()
{
    if (m_shutdownStarted)
    {
        return;
    }
    m_shutdownStarted = true;
    m_stopRequested = true;
    m_audioQueue.Close();

    if (m_listener)
    {
        boost::system::error_code errorCode;
        m_listener->close(errorCode);
    }

    SendDisconnectToClients();

    {
        std::lock_guard lock(m_clientsMutex);
        for (const auto& socket : m_clients)
        {
            if (!socket)
            {
                continue;
            }
            boost::system::error_code errorCode;
            socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, errorCode);
            socket->close(errorCode);
        }
        m_clients.clear();
    }

    m_ioContext.stop();

    if (m_acceptThread.joinable())
    {
        m_acceptThread.join();
    }
    if (m_audioSendThread.joinable())
    {
        m_audioSendThread.join();
    }
}

void Station::BroadcastAudio(const AudioChunk& chunk)
{
    const std::vector<std::byte> payload = SerializeAudioSamples(chunk.samples);
    const PacketHeader header{
        .type = PacketType::AudioChunk,
        .payloadSize = static_cast<uint32_t>(payload.size()),
        .sequence = m_audioSequence++,
        .timestampUs = chunk.timestampUs,
    };

    std::vector<size_t> deadIndices;
    std::lock_guard lock(m_clientsMutex);
    for (size_t index = 0; index < m_clients.size(); ++index)
    {
        if (!SendPacket(*m_clients[index], header, payload))
        {
            deadIndices.push_back(index);
        }
    }
    RemoveDeadSockets(m_clients, deadIndices);
}

void Station::SendDisconnectToClients()
{
    const PacketHeader header{
        .type = PacketType::Disconnect,
        .sequence = m_audioSequence,
    };

    std::lock_guard lock(m_clientsMutex);
    for (const auto& socket : m_clients)
    {
        if (!socket)
        {
            continue;
        }
        static_cast<void>(SendPacket(*socket, header, {}));
    }
}

void Station::RemoveDeadSockets(std::vector<SocketPtr>& sockets, const std::vector<size_t>& deadIndices)
{
    for (auto it = deadIndices.rbegin(); it != deadIndices.rend(); ++it)
    {
        boost::system::error_code errorCode;
        sockets[*it]->close(errorCode);
        sockets.erase(sockets.begin() + static_cast<std::ptrdiff_t>(*it));
    }
}
}
