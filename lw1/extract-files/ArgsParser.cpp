#include "ArgsParser.h"

#include <iostream>
#include <string>

namespace
{

void PrintUsage()
{
	std::cout << "Usage:\n";
	std::cout << "extract-files -S ARCHIVE-NAME [INPUT-FILES...]\n";
	std::cout << "extract-files -P NUM-PROCESSES ARCHIVE-NAME [INPUT-FILES...]\n";
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
		PrintUsage();
		return std::nullopt;
	}

	Args args;
	std::string modeFlag = argv[1];

	if (modeFlag == "-S")
	{
		args.mode = Mode::Sequential;
		args.archiveName = argv[2];
		args.outputFolder = argv[3];
	}
	else if (modeFlag == "-P")
	{
		args.mode = Mode::Parallel;

		if (argc < 5)
		{
			std::cerr << "Error: Missing NUM-PROCESSES after -P\n";
			PrintUsage();
			return std::nullopt;
		}

		if (!IsValidPositiveInt(argv[2], args.numProcesses))
		{
			std::cerr << "Error: NUM-PROCESSES must be a positive integer\n";
			PrintUsage();
			return std::nullopt;
		}

		args.archiveName = argv[3];
		args.outputFolder = argv[4];
	}
	else
	{
		std::cerr << "Error: Unknown mode flag '" << modeFlag << "'. Expected -S or -P\n";
		PrintUsage();
		return std::nullopt;
	}

	return args;
}