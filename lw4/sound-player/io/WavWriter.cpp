#include "io/WavWriter.h"
#include "model/Types.h"

#include <stdexcept>

namespace
{
void WriteUint16(std::ofstream& output, const std::uint16_t value)
{
	output.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void WriteUint32(std::ofstream& output, const std::uint32_t value)
{
	output.write(reinterpret_cast<const char*>(&value), sizeof(value));
}
} // namespace

void wavWriter::Save(const std::string& path, const std::vector<std::int16_t>& samples)
{
	std::ofstream output(path, std::ios::binary);
	if (!output.is_open())
	{
		throw std::runtime_error("Failed to create wav file: " + path);
	}

	const auto dataSize = static_cast<std::uint32_t>(samples.size() * sizeof(std::int16_t));
	const std::uint32_t riffChunkSize = 36u + dataSize;
	constexpr std::uint16_t bitsPerSample = 16;
	constexpr std::uint32_t byteRate = SAMPLE_RATE * CHANNEL_COUNT * bitsPerSample / 8u;
	constexpr std::uint16_t blockAlign = CHANNEL_COUNT * bitsPerSample / 8u;

	output.write("RIFF", 4);
	WriteUint32(output, riffChunkSize);
	output.write("WAVE", 4);

	output.write("fmt ", 4);
	WriteUint32(output, 16);
	WriteUint16(output, 1);
	WriteUint16(output, CHANNEL_COUNT);
	WriteUint32(output, SAMPLE_RATE);
	WriteUint32(output, byteRate);
	WriteUint16(output, blockAlign);
	WriteUint16(output, bitsPerSample);

	output.write("data", 4);
	WriteUint32(output, dataSize);
	output.write(reinterpret_cast<const char*>(samples.data()), dataSize);
}
