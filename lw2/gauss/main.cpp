#include "ArgsParser.h"
#include "GaussianBlur.h"
#include "ImageLoader.h"

#include <iostream>

int main(int argc, char* argv[])
{
	argsParser::Args args = argsParser::ParseArguments(argc, argv);
	if (!args.valid)
	{
		argsParser::PrintUsage();
		return EXIT_FAILURE;
	}

	ImageLoader loader;
	if (!loader.LoadImage(args.inputFile))
	{
		std::cerr << "Failed to load image: " << args.inputFile << std::endl;
		return EXIT_FAILURE;
	}

	auto totalStart = std::chrono::high_resolution_clock::now();

	GaussianBlur blur;
	blur.SetRadius(args.radius);
	blur.SetNumThreads(args.numThreads);
	blur.GenerateKernel();

	auto filterStart = std::chrono::high_resolution_clock::now();

	std::vector<Pixel>& imageData = loader.GetData();
	blur.Process(imageData, loader.GetWidth(), loader.GetHeight());

	auto filterEnd = std::chrono::high_resolution_clock::now();

	if (!ImageLoader::SaveImage(
			args.outputFile,
			loader.GetData(),
			loader.GetWidth(),
			loader.GetHeight()))
	{
		std::cerr << "Failed to save file" << std::endl;
		return EXIT_FAILURE;
	}

	auto totalEnd = std::chrono::high_resolution_clock::now();

	std::chrono::duration<double> filterTime = filterEnd - filterStart;
	std::chrono::duration<double> totalTime = totalEnd - totalStart;

	std::cout << "-----------------------------" << std::endl;
	std::cout << "Filter apply time: " << filterTime.count() << " sec." << std::endl;
	std::cout << "Total time: " << totalTime.count() << " sec." << std::endl;

	return EXIT_SUCCESS;
}