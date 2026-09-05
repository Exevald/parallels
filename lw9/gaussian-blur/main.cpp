#include "GaussianBlurWindow.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace
{

std::filesystem::path GetOutputPath(const std::filesystem::path& input)
{
	return input.parent_path() / (input.stem().string() + "-blurred.png");
}

} // namespace

int main(const int argc, char* argv[])
{
	try
	{
		if (argc < 2 || argc > 3)
		{
			std::cerr << "Usage: gaussian-blur <input-image> [output.png]\n";
			return EXIT_FAILURE;
		}

		const std::filesystem::path inputPath = argv[1];
		const std::filesystem::path outputPath = argc == 3
			? std::filesystem::path(argv[2])
			: GetOutputPath(inputPath);
		GaussianBlurWindow(inputPath, outputPath).Run();
		return EXIT_SUCCESS;
	}
	catch (const std::exception& error)
	{
		std::cerr << "Error: " << error.what() << '\n';
		return EXIT_FAILURE;
	}
}
