#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace radio
{

[[nodiscard]] inline std::array<std::byte, 2> ToBigEndian16(uint16_t value)
{
	return {
		static_cast<std::byte>((value >> 8) & 0xFFU),
		static_cast<std::byte>(value & 0xFFU),
	};
}

[[nodiscard]] inline std::array<std::byte, 4> ToBigEndian32(uint32_t value)
{
	return {
		static_cast<std::byte>((value >> 24) & 0xFFU),
		static_cast<std::byte>((value >> 16) & 0xFFU),
		static_cast<std::byte>((value >> 8) & 0xFFU),
		static_cast<std::byte>(value & 0xFFU),
	};
}

[[nodiscard]] inline std::array<std::byte, 8> ToBigEndian64(uint64_t value)
{
	return {
		static_cast<std::byte>((value >> 56) & 0xFFU),
		static_cast<std::byte>((value >> 48) & 0xFFU),
		static_cast<std::byte>((value >> 40) & 0xFFU),
		static_cast<std::byte>((value >> 32) & 0xFFU),
		static_cast<std::byte>((value >> 24) & 0xFFU),
		static_cast<std::byte>((value >> 16) & 0xFFU),
		static_cast<std::byte>((value >> 8) & 0xFFU),
		static_cast<std::byte>(value & 0xFFU),
	};
}

[[nodiscard]] inline uint16_t FromBigEndian16(const std::byte* data)
{
	return (static_cast<uint16_t>(std::to_integer<uint8_t>(data[0])) << 8)
		| static_cast<uint16_t>(std::to_integer<uint8_t>(data[1]));
}

[[nodiscard]] inline uint32_t FromBigEndian32(const std::byte* data)
{
	return (static_cast<uint32_t>(std::to_integer<uint8_t>(data[0])) << 24)
		| (static_cast<uint32_t>(std::to_integer<uint8_t>(data[1])) << 16)
		| (static_cast<uint32_t>(std::to_integer<uint8_t>(data[2])) << 8)
		| static_cast<uint32_t>(std::to_integer<uint8_t>(data[3]));
}

[[nodiscard]] inline uint64_t FromBigEndian64(const std::byte* data)
{
	return (static_cast<uint64_t>(std::to_integer<uint8_t>(data[0])) << 56)
		| (static_cast<uint64_t>(std::to_integer<uint8_t>(data[1])) << 48)
		| (static_cast<uint64_t>(std::to_integer<uint8_t>(data[2])) << 40)
		| (static_cast<uint64_t>(std::to_integer<uint8_t>(data[3])) << 32)
		| (static_cast<uint64_t>(std::to_integer<uint8_t>(data[4])) << 24)
		| (static_cast<uint64_t>(std::to_integer<uint8_t>(data[5])) << 16)
		| (static_cast<uint64_t>(std::to_integer<uint8_t>(data[6])) << 8)
		| static_cast<uint64_t>(std::to_integer<uint8_t>(data[7]));
}

inline void AppendBigEndian16(std::vector<std::byte>& output, uint16_t value)
{
	const auto bytes = ToBigEndian16(value);
	output.insert(output.end(), bytes.begin(), bytes.end());
}

inline void AppendBigEndian32(std::vector<std::byte>& output, uint32_t value)
{
	const auto bytes = ToBigEndian32(value);
	output.insert(output.end(), bytes.begin(), bytes.end());
}

inline void AppendBigEndian64(std::vector<std::byte>& output, uint64_t value)
{
	const auto bytes = ToBigEndian64(value);
	output.insert(output.end(), bytes.begin(), bytes.end());
}

} // namespace radio
