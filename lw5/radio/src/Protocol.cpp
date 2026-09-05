#include "Protocol.h"

#include "ByteOrder.h"

#include <stdexcept>

namespace radio
{

namespace
{

constexpr size_t PacketHeaderSize = 28;
constexpr size_t StreamConfigSize = 8;

} // namespace

std::vector<std::byte> SerializePacketHeader(const PacketHeader& header)
{
	std::vector<std::byte> output;
	output.reserve(PacketHeaderSize);
	AppendBigEndian32(output, header.magic);
	AppendBigEndian16(output, header.version);
	AppendBigEndian16(output, static_cast<uint16_t>(header.type));
	AppendBigEndian32(output, header.payloadSize);
	AppendBigEndian64(output, header.sequence);
	AppendBigEndian64(output, header.timestampUs);
	return output;
}

PacketHeader DeserializePacketHeader(const std::vector<std::byte>& bytes)
{
	if (bytes.size() != PacketHeaderSize)
	{
		throw std::runtime_error("Invalid packet header size");
	}

	PacketHeader header;
	header.magic = FromBigEndian32(bytes.data());
	header.version = FromBigEndian16(bytes.data() + 4);
	header.type = static_cast<PacketType>(FromBigEndian16(bytes.data() + 6));
	header.payloadSize = FromBigEndian32(bytes.data() + 8);
	header.sequence = FromBigEndian64(bytes.data() + 12);
	header.timestampUs = FromBigEndian64(bytes.data() + 20);

	if (header.magic != PacketMagic)
	{
		throw std::runtime_error("Invalid packet magic");
	}

	return header;
}

std::vector<std::byte> SerializeStreamConfig(const StreamConfig& config)
{
	std::vector<std::byte> output;
	output.reserve(StreamConfigSize);
	AppendBigEndian16(output, config.audioChannels);
	AppendBigEndian16(output, config.audioBitsPerSample);
	AppendBigEndian32(output, config.audioSampleRate);
	return output;
}

StreamConfig DeserializeStreamConfig(const std::vector<std::byte>& bytes)
{
	if (bytes.size() != StreamConfigSize)
	{
		throw std::runtime_error("Invalid stream config size");
	}

	StreamConfig config;
	config.audioChannels = FromBigEndian16(bytes.data());
	config.audioBitsPerSample = FromBigEndian16(bytes.data() + 2);
	config.audioSampleRate = FromBigEndian32(bytes.data() + 4);
	return config;
}

std::vector<std::byte> SerializeAudioSamples(const std::vector<int16_t>& samples)
{
	std::vector<std::byte> output;
	output.reserve(samples.size() * 2);
	for (int16_t sample : samples)
	{
		AppendBigEndian16(output, static_cast<uint16_t>(sample));
	}
	return output;
}

std::vector<int16_t> DeserializeAudioSamples(const std::vector<std::byte>& bytes)
{
	if (bytes.size() % 2 != 0)
	{
		throw std::runtime_error("Invalid audio payload size");
	}

	std::vector<int16_t> samples(bytes.size() / 2);
	for (size_t index = 0; index < samples.size(); ++index)
	{
		samples[index] = static_cast<int16_t>(FromBigEndian16(bytes.data() + index * 2));
	}
	return samples;
}

std::string PacketTypeToString(PacketType type)
{
	switch (type)
	{
	case PacketType::StreamConfig:
		return "StreamConfig";
	case PacketType::AudioChunk:
		return "AudioChunk";
	case PacketType::Disconnect:
		return "Disconnect";
	}
	return "Unknown";
}

} // namespace radio
