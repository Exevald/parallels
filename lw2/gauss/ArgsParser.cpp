#include "ArgsParser.h"

argsParser::Args argsParser::ParseArguments(int argc, char* argv[])
{
	argsParser::Args args;

	if (argc < 3)
	{
		return args;
	}

	args.inputFile = argv[1];
	args.outputFile = argv[2];

	if (argc >= 4)
	{
		try
		{
			args.radius = std::stoi(argv[3]);
		}
		catch (...)
		{
			args.radius = 5;
		}
	}

	if (argc >= 5)
	{
		try
		{
			args.numThreads = std::stoi(argv[4]);
		}
		catch (...)
		{
			args.numThreads = 1;
		}
	}

	args.valid = true;
	return args;
}

void argsParser::PrintUsage()
{
	std::cout << "Usage: "
			  << "gauss INPUT_FILE OUTPUT_FILE RADIUS NUM_THREADS" << std::endl;
}