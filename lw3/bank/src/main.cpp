#include "Simulation.h"
#include <iostream>
#include <optional>

struct Args
{
	int iterationsCount;
	bool isConsoleModeEnabled;
};

Args ParseArgs(int argc, char* argv[])
{
	if (argc == 1)
	{
		return Args{ .isConsoleModeEnabled = true };
	}
	if (argc == 2)
	{
		return Args{ .iterationsCount = std::stoi(argv[1]) };
	}
	throw std::invalid_argument("Invalid arguments count\n"
								"Usage: economy [NUM_ITERATIONS]");
}

int main(int argc, char* argv[])
{
	try
	{
		auto args = ParseArgs(argc, argv);
		auto simulation = Simulation();
		int iterationsCount = args.iterationsCount;

		if (args.isConsoleModeEnabled)
		{
			std::cout << "Please, insert iterations count\n";
			std::cin >> iterationsCount;
		}
		simulation.StartSimulation(iterationsCount);
	}
	catch (const std::exception& exception)
	{
		std::cout << exception.what() << "\n";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}