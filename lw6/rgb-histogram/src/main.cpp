#include "Histogram.h"
#include "ImageLoader.h"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
struct Options
{
	std::string inputPath;
	size_t threads = 0;
	std::string mode = "single";
};

void PrintUsage()
{
	std::cout << "Usage: rgb-histogram --input <image> [--threads N] [--mode single|atomic-2a|atomic-2b|local]\n";
}

Options ParseArguments(int argc, char* argv[])
{
	Options options;

	for (int index = 1; index < argc; ++index)
	{
		if (const std::string argument = argv[index]; argument == "--input" && index + 1 < argc)
		{
			options.inputPath = argv[++index];
		}
		else if (argument == "--threads" && index + 1 < argc)
		{
			options.threads = std::stoul(argv[++index]);
		}
		else if (argument == "--mode" && index + 1 < argc)
		{
			options.mode = argv[++index];
		}
		else
		{
			throw std::invalid_argument("Unknown or incomplete argument: " + argument);
		}
	}

	if (options.inputPath.empty())
	{
		throw std::invalid_argument("Missing required argument --input");
	}

	return options;
}

HistogramResult BuildHistogram(const RgbImage& image, const Options& options)
{
	if (options.mode == "single")
	{
		return BuildHistogramSingleThread(image);
	}
	if (options.mode == "atomic-2a")
	{
		return BuildHistogramAtomic(image, options.threads, AtomicLayout::InterleavedRgb);
	}
	if (options.mode == "atomic-2b")
	{
		return BuildHistogramAtomic(image, options.threads, AtomicLayout::ChannelBlocks);
	}
	if (options.mode == "local")
	{
		return BuildHistogramLocal(image, options.threads);
	}

	throw std::invalid_argument("Unsupported mode: " + options.mode);
}

float SumChannel(const std::array<float, 256>& values)
{
	float sum = 0.0f;
	for (float value : values)
	{
		sum += value;
	}
	return sum;
}
} // namespace

int main(const int argc, char* argv[])
{
	try
	{
		const Options options = ParseArguments(argc, argv);
		const RgbImage image = ImageLoader::LoadRgbImage(options.inputPath);

		const auto start = std::chrono::steady_clock::now();
		const HistogramResult histogram = BuildHistogram(image, options);
		const auto finish = std::chrono::steady_clock::now();

		const std::chrono::duration<double, std::milli> elapsed = finish - start;

		std::cout << "Pixels: " << image.PixelCount() << "\n";
		std::cout << "Threads: " << ResolveThreadCount(options.threads, image.PixelCount()) << "\n";
		std::cout << "Mode: " << options.mode << "\n";
		std::cout << "Time: " << elapsed.count() << " ms\n";
		std::cout << "Sum(R): " << SumChannel(histogram.red) << "\n";
		std::cout << "Sum(G): " << SumChannel(histogram.green) << "\n";
		std::cout << "Sum(B): " << SumChannel(histogram.blue) << "\n";
	}
	catch (const std::exception& exception)
	{
		std::cerr << exception.what() << "\n";
		PrintUsage();
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}