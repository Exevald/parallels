#include "Receiver.h"

#include <boost/asio/ip/address.hpp>
#include <cstdlib>
#include <iostream>

namespace radio
{

Receiver::Receiver(Options options, std::atomic_bool& stopRequested)
	: m_options(std::move(options))
	, m_stopRequested(stopRequested)
{
}

Receiver::~Receiver()
{
	Shutdown();
}

int Receiver::Run()
{
	if (!Connect())
	{
		Shutdown();
		return EXIT_FAILURE;
	}

	m_audioStream.Configure(m_streamConfig.audioChannels, m_streamConfig.audioSampleRate);
	m_audioStream.Start();
	m_receiveThread = std::thread(&Receiver::ReceiveLoop, this);

	std::cout << "radio receiver connected\n";
	std::cout << "sample rate: " << m_streamConfig.audioSampleRate << "\n";
	std::cout << "channels: " << m_streamConfig.audioChannels << "\n";

	m_visualizer->Run(m_stopRequested);

	Shutdown();
	return EXIT_SUCCESS;
}

bool Receiver::Connect()
{
	boost::system::error_code errorCode;
	const auto address = boost::asio::ip::make_address(m_options.address, errorCode);
	if (errorCode)
	{
		std::cerr << "Invalid receiver address\n";
		return false;
	}

	m_socket.connect({ address, m_options.port }, errorCode);
	if (errorCode)
	{
		std::cerr << "Failed to connect to station\n";
		return false;
	}

	ReceivedPacket packet;
	if (!ReceivePacket(m_socket, packet))
	{
		std::cerr << "Failed to receive stream config\n";
		return false;
	}
	if (packet.header.type != PacketType::StreamConfig)
	{
		std::cerr << "Unexpected first packet: " << PacketTypeToString(packet.header.type) << "\n";
		return false;
	}

	m_streamConfig = DeserializeStreamConfig(packet.payload);
	m_visualizer = std::make_unique<AudioVisualizer>("radio receiver", m_streamConfig.audioChannels);
	return true;
}

void Receiver::ReceiveLoop()
{
	while (!m_stopRequested.load())
	{
		ReceivedPacket packet;
		if (!ReceivePacket(m_socket, packet))
		{
			if (!m_stopRequested.load())
			{
				std::cerr << "Station disconnected\n";
			}
			m_stopRequested = true;
			break;
		}

		if (packet.header.type == PacketType::Disconnect)
		{
			std::cout << "Station stopped\n";
			m_stopRequested = true;
			break;
		}
		if (packet.header.type != PacketType::AudioChunk)
		{
			continue;
		}

		AudioChunk chunk;
		chunk.timestampUs = packet.header.timestampUs;
		chunk.samples = DeserializeAudioSamples(packet.payload);
		m_visualizer->UpdateSamples(chunk.samples);
		m_audioStream.Enqueue(std::move(chunk));
	}
}

void Receiver::Shutdown()
{
	if (m_shutdownStarted)
	{
		return;
	}
	m_shutdownStarted = true;
	m_stopRequested = true;
	m_audioStream.Close();

	boost::system::error_code errorCode;
	m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, errorCode);
	m_socket.close(errorCode);
	m_ioContext.stop();

	if (m_receiveThread.joinable())
	{
		m_receiveThread.join();
	}
}

} // namespace radio
