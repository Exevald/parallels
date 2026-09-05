#include "AudioRecorder.h"

namespace radio
{
NetworkAudioRecorder::NetworkAudioRecorder(Callback callback)
    : m_callback(std::move(callback))
{
}

NetworkAudioRecorder::~NetworkAudioRecorder()
{
    stop();
}

bool NetworkAudioRecorder::onStart()
{
    m_startTime = std::chrono::steady_clock::now();
    return true;
}

bool NetworkAudioRecorder::onProcessSamples(const std::int16_t* samples, std::size_t sampleCount)
{
    AudioChunk chunk;
    chunk.samples.assign(samples, samples + sampleCount);
    chunk.timestampUs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - m_startTime)
            .count());
    m_callback(std::move(chunk));
    return true;
}

}
