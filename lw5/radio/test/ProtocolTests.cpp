#include "Protocol.h"

#include <gtest/gtest.h>

TEST(Protocol, PacketHeaderRoundTrip)
{
	constexpr radio::PacketHeader header{
		.type = radio::PacketType::AudioChunk,
		.payloadSize = 1024,
		.sequence = 55,
		.timestampUs = 123456789,
	};

	const auto bytes = radio::SerializePacketHeader(header);
	const radio::PacketHeader roundTrip = radio::DeserializePacketHeader(bytes);

	EXPECT_EQ(roundTrip.magic, radio::PacketMagic);
	EXPECT_EQ(roundTrip.version, radio::ProtocolVersion);
	EXPECT_EQ(roundTrip.type, radio::PacketType::AudioChunk);
	EXPECT_EQ(roundTrip.payloadSize, 1024U);
	EXPECT_EQ(roundTrip.sequence, 55U);
	EXPECT_EQ(roundTrip.timestampUs, 123456789U);
}

TEST(Protocol, StreamConfigRoundTrip)
{
	constexpr radio::StreamConfig config{
		.audioChannels = 2,
		.audioBitsPerSample = 16,
		.audioSampleRate = 48000,
	};

	const auto bytes = radio::SerializeStreamConfig(config);
	const radio::StreamConfig roundTrip = radio::DeserializeStreamConfig(bytes);

	EXPECT_EQ(roundTrip.audioChannels, 2);
	EXPECT_EQ(roundTrip.audioBitsPerSample, 16);
	EXPECT_EQ(roundTrip.audioSampleRate, 48000U);
}

TEST(Protocol, AudioSamplesRoundTrip)
{
	const std::vector<int16_t> samples{ 0, 1, -1, 32767, -32768 };
	const auto bytes = radio::SerializeAudioSamples(samples);
	const auto roundTrip = radio::DeserializeAudioSamples(bytes);

	EXPECT_EQ(roundTrip, samples);
}

TEST(Protocol, DisconnectPacketName)
{
	EXPECT_EQ(radio::PacketTypeToString(radio::PacketType::Disconnect), "Disconnect");
}
