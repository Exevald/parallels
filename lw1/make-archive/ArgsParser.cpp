#include "ArgsParser.h"

#include <iostream>
#include <string>

namespace
{

void PrintUsage(const char* programName)
{
	std::cout << "Usage:\n";
	std::cout << "  " << programName << " -S ARCHIVE-NAME [INPUT-FILES...]\n";
	std::cout << "  " << programName << " -P NUM-PROCESSES ARCHIVE-NAME [INPUT-FILES...]\n";
}

bool IsValidPositiveInt(const std::string& str, int& result)
{
	try
	{
		size_t pos = 0;
		result = std::stoi(str, &pos);
		return pos == str.length() && result > 0;
	}
	catch (...)
	{
		return false;
	}
}

} // namespace

std::optional<argsParser::Args> argsParser::ParseArgs(int argc, char* argv[])
{
	if (argc < 4)
	{
		PrintUsage(argv[0]);
		return std::nullopt;
	}

	Args args;
	std::string modeFlag = argv[1];

	if (modeFlag == "-S")
	{
		args.mode = Mode::Sequential;
		args.archiveName = argv[2];

		for (int i = 3; i < argc; ++i)
		{
			args.inputFiles.emplace_back(argv[i]);
		}
	}
	else if (modeFlag == "-P")
	{
		args.mode = Mode::Parallel;

		if (argc < 5)
		{
			std::cerr << "Error: Missing NUM-PROCESSES after -P\n";
			PrintUsage(argv[0]);
			return std::nullopt;
		}

		if (!IsValidPositiveInt(argv[2], args.numProcesses))
		{
			std::cerr << "Error: NUM-PROCESSES must be a positive integer\n";
			PrintUsage(argv[0]);
			return std::nullopt;
		}

		args.archiveName = argv[3];
		for (int i = 4; i < argc; ++i)
		{
			args.inputFiles.emplace_back(argv[i]);
		}
	}
	else
	{
		std::cerr << "Error: Unknown mode flag '" << modeFlag << "'. Expected -S or -P\n";
		PrintUsage(argv[0]);
		return std::nullopt;
	}

	if (args.inputFiles.empty())
	{
		std::cerr << "Error: No input files specified\n";
		PrintUsage(argv[0]);
		return std::nullopt;
	}

	return args;
}