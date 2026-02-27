#include "Simulation.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
	try
	{
		int iterations = 1000;
		bool parallel = true;

		if (argc > 1)
		{
			iterations = std::stoi(argv[1]);
		}

		Simulation sim(100000);

		if (parallel)
		{
			std::cout << "Starting PARALLEL simulation: " << iterations << " iterations per actor...\n";
			sim.RunParallel(iterations);
		}
		else
		{
			std::cout << "Starting SEQUENTIAL simulation: " << iterations << " total cycles...\n";
			sim.RunSequential(iterations);
		}

		std::cout << "Simulation finished. Total Bank Operations: " << sim.GetTotalOps() << "\n";

		if (sim.IsStateConsistent())
		{
			return 0;
		}
		else
		{
			return 1;
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
}