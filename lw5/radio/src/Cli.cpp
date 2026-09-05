#include "Cli.h"

#include <iostream>
#include <stdexcept>

namespace radio
{
namespace
{
[[nodiscard]] uint16_t ParsePort(const std::string& value)
{
	const unsigned long parsed = std::stoul(value);
	if (parsed == 0 || parsed > 65535)
	{
		throw std::invalid_argument("Port must be in range [1, 65535]");
	}
	return static_cast<uint16_t>(parsed);
}
} // namespace

Options ParseOptions(int argc, char* argv[])
{
	if (argc == 2)
	{
		return Options{
			.mode = RunMode::Station,
			.port = ParsePort(argv[1]),
		};
	}

	if (argc == 3)
	{
		return Options{
			.mode = RunMode::Receiver,
			.address = argv[1],
			.port = ParsePort(argv[2]),
		};
	}

	throw std::invalid_argument("Invalid arguments");
}

void PrintUsage()
{
	std::cout << "Station mode:  radio PORT\n";
	std::cout << "Receiver mode: radio ADDRESS PORT\n";
}
} // namespace radio
