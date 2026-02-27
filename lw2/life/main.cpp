#include "LifeCli.h"

#include <iostream>
#include <string>
#include <vector>

int main(int argc, char* argv[])
{
	try
	{
		std::vector<std::string> args(argv, argv + argc);
		if (args.size() < 2)
		{
			LifeCli::ShowUsage();
			return 1;
		}

		std::string mode = args[1];

		if (mode == "generate")
		{
			return LifeCli::RunGenerateMode(args);
		}
		else if (mode == "step")
		{
			return LifeCli::RunStepMode(args);
		}
		else
		{
			std::cerr << "Unknown mode '" << mode << "'\n\n";
			LifeCli::ShowUsage();
			return 1;
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << "\n";
		return 1;
	}
}