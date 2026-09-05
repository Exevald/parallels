#pragma once

#include "Protocol.h"

#include <SFML/Audio/SoundRecorder.hpp>
#include <chrono>
#include <functional>

namespace radio
{

class NetworkAudioRecorder : public sf::SoundRecorder
{
public:
    using Callback = std::function<void(AudioChunk)>;

    explicit NetworkAudioRecorder(Callback callback);
    ~NetworkAudioRecorder() override;

protected:
    bool onStart() override;
    [[nodiscard]] bool onProcessSamples(const std::int16_t* samples, std::size_t sampleCount) override;

private:
    Callback m_callback;
    std::chrono::steady_clock::time_point m_startTime;
};

}
