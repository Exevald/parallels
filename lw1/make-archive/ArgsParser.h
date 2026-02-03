#pragma once

#include <optional>
#include <string>
#include <vector>

namespace argsParser
{

enum class Mode
{
	Sequential,
	Parallel
};

struct Args
{
	Mode mode;
	std::string archiveName;
	std::vector<std::string> inputFiles;
	int numProcesses = 1;
};

std::optional<Args> ParseArgs(int argc, char* argv[]);
} // namespace ArgsParser