#pragma once

#include "ConcurrentQueue.h"
#include "Protocol.h"

#include <SFML/Audio/SoundStream.hpp>

#include <cstddef>
#include <mutex>

namespace radio
{

class AudioPlaybackStream : sf::SoundStream
{
public:
	AudioPlaybackStream() = default;
	~AudioPlaybackStream() override;

	void Configure(unsigned int channelCount, unsigned int sampleRate);
	void Start();
	void Enqueue(AudioChunk chunk);
	void Close();

private:
	static constexpr std::size_t PrebufferChunkCount = 4;

	[[nodiscard]] bool onGetData(Chunk& data) override;
	void onSeek(sf::Time) override;

	BoundedQueue<AudioChunk> m_queue{ 64 };
	std::mutex m_stateMutex;
	AudioChunk m_currentChunk;
	bool m_startRequested = false;
	bool m_started = false;
	bool m_closed = false;
};

} // namespace radio
