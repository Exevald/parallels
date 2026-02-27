#include "LifeCli.h"
#include "LifeSimulator.h"

#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

void LifeCli::ShowUsage()
{
	std::cout << "Usage:\n";
	std::cout << "  ./life generate OUTPUT_FILE WIDTH HEIGHT PROBABILITY\n";
	std::cout << "  ./life step INPUT_FILE NUM_THREADS [OUTPUT_FILE]\n";
	std::cout << "  ./life visualize INPUT_FILE NUM_THREADS\n";
	std::cout << "\nExamples:\n";
	std::cout << "  ./life generate start.txt 100 100 0.3\n";
	std::cout << "  ./life step start.txt 4 next.txt\n";
	std::cout << "  ./life visualize start.txt 4\n";
}

int LifeCli::RunGenerateMode(const std::vector<std::string>& args)
{
	if (args.size() != 6)
	{
		std::cerr << "Error: Invalid arguments for 'generate' mode\n";
		ShowUsage();
		return 1;
	}

	const std::string& outputFile = args[2];
	int width = std::stoi(args[3]);
	int height = std::stoi(args[4]);
	double probability = std::stod(args[5]);

	try
	{
		LifeSimulator simulator(width, height);
		simulator.GetGrid().GenerateRandom(probability);
		simulator.SaveState(outputFile);
		std::cout << "Generated grid " << width << "x" << height
				  << " to " << outputFile << "\n";
		return 0;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error during generation: " << e.what() << "\n";
		return 1;
	}
}

int LifeCli::RunStepMode(const std::vector<std::string>& args)
{
	if (args.size() < 4 || args.size() > 5)
	{
		std::cerr << "Error: Invalid arguments for 'step' mode\n";
		ShowUsage();
		return 1;
	}

	std::string inputFile = args[2];
	size_t numThreads = std::stoul(args[3]);
	std::string outputFile = (args.size() == 5) ? args[4] : inputFile;

	try
	{
		LifeSimulator simulator(inputFile);
		double computationTime = simulator.ComputeNextGeneration(numThreads);
		simulator.SaveState(outputFile);
		std::cout << std::fixed << std::setprecision(6) << computationTime << "\n";
		return 0;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error during step computation: " << e.what() << "\n";
		return 1;
	}
}