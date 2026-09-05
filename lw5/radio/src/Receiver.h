#pragma once

#include "AudioPlaybackStream.h"
#include "AudioVisualizer.h"
#include "Cli.h"
#include "Protocol.h"
#include "TcpChannel.h"

#include <atomic>
#include <boost/asio/io_context.hpp>
#include <memory>
#include <thread>

namespace radio
{
class Receiver
{
public:
	Receiver(Options options, std::atomic_bool& stopRequested);
	~Receiver();

	int Run();

private:
	[[nodiscard]] bool Connect();
	void ReceiveLoop();
	void Shutdown();

	const Options m_options;
	std::atomic_bool& m_stopRequested;
	StreamConfig m_streamConfig;
	boost::asio::io_context m_ioContext;
	TcpSocket m_socket{ m_ioContext };
	AudioPlaybackStream m_audioStream;
	std::thread m_receiveThread;
	bool m_shutdownStarted = false;
	std::unique_ptr<AudioVisualizer> m_visualizer;
};
} // namespace radio
