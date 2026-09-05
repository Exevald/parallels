#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace radio
{

constexpr uint32_t PacketMagic = 0x54565456U;
constexpr uint16_t ProtocolVersion = 1;

enum class PacketType : uint16_t
{
    StreamConfig = 1,
    AudioChunk = 2,
    Disconnect = 3,
};

struct PacketHeader
{
    uint32_t magic = PacketMagic;
    uint16_t version = ProtocolVersion;
    PacketType type = PacketType::StreamConfig;
    uint32_t payloadSize = 0;
    uint64_t sequence = 0;
    uint64_t timestampUs = 0;
};

struct StreamConfig
{
    uint16_t audioChannels = 1;
    uint16_t audioBitsPerSample = 16;
    uint32_t audioSampleRate = 44100;
};

struct AudioChunk
{
    uint64_t timestampUs = 0;
    std::vector<int16_t> samples;
};

[[nodiscard]] std::vector<std::byte> SerializePacketHeader(const PacketHeader& header);
[[nodiscard]] PacketHeader DeserializePacketHeader(const std::vector<std::byte>& bytes);

[[nodiscard]] std::vector<std::byte> SerializeStreamConfig(const StreamConfig& config);
[[nodiscard]] StreamConfig DeserializeStreamConfig(const std::vector<std::byte>& bytes);

[[nodiscard]] std::vector<std::byte> SerializeAudioSamples(const std::vector<int16_t>& samples);
[[nodiscard]] std::vector<int16_t> DeserializeAudioSamples(const std::vector<std::byte>& bytes);

[[nodiscard]] std::string PacketTypeToString(PacketType type);

}
