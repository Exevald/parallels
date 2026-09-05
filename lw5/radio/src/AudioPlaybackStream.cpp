#include "AudioPlaybackStream.h"

#include <SFML/Audio/SoundChannel.hpp>
#include <vector>

namespace radio
{
AudioPlaybackStream::~AudioPlaybackStream()
{
    Close();
}

void AudioPlaybackStream::Configure(const unsigned int channelCount, const unsigned int sampleRate)
{
    std::vector<sf::SoundChannel> channelMap;
    if (channelCount == 1)
    {
        channelMap = {sf::SoundChannel::Mono};
    }
    else
    {
        channelMap = {
            sf::SoundChannel::FrontLeft,
            sf::SoundChannel::FrontRight,
        };
    }
    initialize(channelCount, sampleRate, channelMap);
}

void AudioPlaybackStream::Start()
{
    std::lock_guard lock(m_stateMutex);
    m_startRequested = true;
}

void AudioPlaybackStream::Enqueue(AudioChunk chunk)
{
    m_queue.PushDropOldest(std::move(chunk));

    std::lock_guard lock(m_stateMutex);
    if (!m_closed && m_startRequested && !m_started && m_queue.Size() >= PrebufferChunkCount)
    {
        m_started = true;
        play();
    }
}

void AudioPlaybackStream::Close()
{
    {
        std::lock_guard lock(m_stateMutex);
        if (m_closed)
        {
            return;
        }
        m_closed = true;
    }

    m_queue.Close();
    stop();
}

bool AudioPlaybackStream::onGetData(Chunk& data)
{
    const auto chunk = m_queue.WaitPop();
    if (!chunk.has_value())
    {
        return false;
    }

    m_currentChunk = std::move(*chunk);
    data.samples = m_currentChunk.samples.data();
    data.sampleCount = m_currentChunk.samples.size();
    return !m_currentChunk.samples.empty();
}

void AudioPlaybackStream::onSeek(sf::Time)
{
}

}
