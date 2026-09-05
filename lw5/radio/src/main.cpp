#include "Cli.h"
#include "Receiver.h"
#include "Station.h"

#include <atomic>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <iostream>

namespace
{

std::atomic_bool g_stopRequested = false;

void HandleSignal(int)
{
	g_stopRequested = true;
}

} // namespace

int main(int argc, char* argv[])
{
	std::signal(SIGINT, HandleSignal);
	std::signal(SIGTERM, HandleSignal);

	try
	{
		const radio::Options options = radio::ParseOptions(argc, argv);
		if (options.mode == radio::RunMode::Station)
		{
			radio::Station station(options, g_stopRequested);
			return station.Run();
		}

		radio::Receiver receiver(options, g_stopRequested);
		return receiver.Run();
	}
	catch (const std::exception& exception)
	{
		std::cerr << exception.what() << "\n";
		radio::PrintUsage();
		return EXIT_FAILURE;
	}
}
