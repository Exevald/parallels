#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <vector>

namespace radio
{
class AudioVisualizer
{
public:
    AudioVisualizer(std::string title, unsigned int channelCount);

    void UpdateSamples(const std::vector<int16_t>& samples);
    void Run(const std::atomic_bool& stopRequested);

private:
    [[nodiscard]] std::vector<int16_t> SnapshotSamples() const;

    std::string m_title;
    unsigned int m_channelCount = 1;
    mutable std::mutex m_samplesMutex;
    std::vector<int16_t> m_recentSamples;
    bool m_screenInitialized = false;
};
}
