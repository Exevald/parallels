#pragma once

#include <cstdint>
#include <string>

namespace radio
{

enum class RunMode
{
	Station,
	Receiver,
};

struct Options
{
	RunMode mode = RunMode::Station;
	std::string address;
	uint16_t port = 0;
};

[[nodiscard]] Options ParseOptions(int argc, char* argv[]);
void PrintUsage();

} // namespace radio
