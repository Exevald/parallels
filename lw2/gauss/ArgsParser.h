#pragma once

#include <iostream>
#include <string>

namespace argsParser
{
struct Args
{
	std::string inputFile;
	std::string outputFile;
	int radius{ 5 };
	int numThreads{ 1 };
	bool valid{ false };
};

Args ParseArguments(int argc, char* argv[]);
void PrintUsage();
} // namespace argsParser