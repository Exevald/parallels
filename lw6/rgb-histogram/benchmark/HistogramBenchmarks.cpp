#include "Histogram.h"
#include "ImageLoader.h"
#include "Timer.h"

#include <array>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace
{
struct BenchmarkCase
{
	std::string name;
	size_t threads = 1;
	enum class Mode
	{
		Single,
		AtomicInterleaved,
		AtomicChannelBlocks,
		Local,
	} mode = Mode::Single;
};

struct BenchmarkOptions
{
	std::string imagePath;
	int width = 6000;
	int height = 4000;
	uint32_t seed = 12345;
	std::vector<int> threads{ 1, 2, 4, 8, 16 };
	int minIterations = 5;
	double minSeconds = 0.2;
};

BenchmarkOptions ParseBenchmarkOptions(int& argc, char* argv[])
{
	BenchmarkOptions options;
	int writeIndex = 1;

	for (int readIndex = 1; readIndex < argc; ++readIndex)
	{
		const std::string_view argument(argv[readIndex]);

		if (argument.rfind("--image=", 0) == 0)
		{
			options.imagePath = std::string(argument.substr(8));
			continue;
		}
		if (argument.rfind("--width=", 0) == 0)
		{
			options.width = std::stoi(std::string(argument.substr(8)));
			continue;
		}
		if (argument.rfind("--height=", 0) == 0)
		{
			options.height = std::stoi(std::string(argument.substr(9)));
			continue;
		}
		if (argument.rfind("--seed=", 0) == 0)
		{
			options.seed = static_cast<uint32_t>(
				std::stoul(std::string(argument.substr(7))));
			continue;
		}
		if (argument.rfind("--threads=", 0) == 0)
		{
			options.threads.clear();
			size_t begin = 10;
			while (begin < argument.size())
			{
				const size_t comma = argument.find(',', begin);
				const size_t end = comma == std::string_view::npos ? argument.size() : comma;
				options.threads.push_back(
					std::stoi(std::string(argument.substr(begin, end - begin))));
				begin = end + 1;
			}
			continue;
		}
		if (argument.rfind("--min-iterations=", 0) == 0)
		{
			options.minIterations = std::stoi(std::string(argument.substr(17)));
			continue;
		}
		if (argument.rfind("--min-seconds=", 0) == 0)
		{
			options.minSeconds = std::stod(std::string(argument.substr(14)));
			continue;
		}

		argv[writeIndex++] = argv[readIndex];
	}

	argc = writeIndex;
	return options;
}

RgbImage PrepareImage(const BenchmarkOptions& options)
{
	if (!options.imagePath.empty())
	{
		return ImageLoader::LoadRgbImage(options.imagePath);
	}
	return ImageLoader::GenerateRandomImage(options.width, options.height, options.seed);
}

std::vector<BenchmarkCase> BuildCases(const BenchmarkOptions& options)
{
	std::vector<BenchmarkCase> cases;
	cases.push_back(BenchmarkCase{ "single_thread", 1, BenchmarkCase::Mode::Single });

	for (const int threads : options.threads)
	{
		const auto threadCount = static_cast<size_t>(threads);
		cases.push_back(BenchmarkCase{
			"atomic_interleaved",
			threadCount, BenchmarkCase::Mode::AtomicInterleaved });
		cases.push_back(BenchmarkCase{
			"atomic_channel_blocks",
			threadCount, BenchmarkCase::Mode::AtomicChannelBlocks });
		cases.push_back(BenchmarkCase{
			"local_histograms",
			threadCount, BenchmarkCase::Mode::Local });
	}

	return cases;
}

HistogramResult RunCase(const BenchmarkCase& benchmarkCase, const RgbImage& image)
{
	switch (benchmarkCase.mode)
	{
	case BenchmarkCase::Mode::Single:
		return BuildHistogramSingleThread(image);
	case BenchmarkCase::Mode::AtomicInterleaved:
		return BuildHistogramAtomic(image, benchmarkCase.threads, AtomicLayout::InterleavedRgb);
	case BenchmarkCase::Mode::AtomicChannelBlocks:
		return BuildHistogramAtomic(image, benchmarkCase.threads, AtomicLayout::ChannelBlocks);
	case BenchmarkCase::Mode::Local:
		return BuildHistogramLocal(image, benchmarkCase.threads);
	}
	throw std::logic_error("Unknown benchmark mode");
}

double SumChannel(const std::array<float, 256>& values)
{
	double sum = 0.0;
	for (float value : values)
	{
		sum += value;
	}
	return sum;
}

double ConsumeHistogram(const HistogramResult& histogram)
{
	return SumChannel(histogram.red) + SumChannel(histogram.green) + SumChannel(histogram.blue);
}

void PrintHeader()
{
	std::cout << std::left
			  << std::setw(34) << "benchmark"
			  << std::setw(10) << "threads"
			  << std::setw(12) << "iters"
			  << std::setw(16) << "total_ms"
			  << std::setw(16) << "avg_ms"
			  << std::setw(16) << "speedup"
			  << '\n';
}

double RunBenchmarkCase(
	const BenchmarkCase& benchmarkCase,
	const RgbImage& image,
	const BenchmarkOptions& options,
	const double baselineMs)
{
	int iterations = 0;
	double totalSeconds = 0.0;
	double sink = 0.0;

	while (iterations < options.minIterations || totalSeconds < options.minSeconds)
	{
		timer::Stopwatch stopwatch;
		const HistogramResult histogram = RunCase(benchmarkCase, image);
		stopwatch.Stop();
		sink += ConsumeHistogram(histogram);
		totalSeconds += stopwatch.ElapsedSeconds();
		++iterations;
	}

	if (sink < 0.0)
	{
		std::cerr << "Unexpected negative histogram checksum\n";
	}

	const double totalMs = totalSeconds * 1000.0;
	const double averageMs = totalMs / static_cast<double>(iterations);
	const double speedup = baselineMs > 0.0 ? baselineMs / averageMs : 1.0;

	std::ostringstream label;
	label << benchmarkCase.name;
	if (benchmarkCase.mode != BenchmarkCase::Mode::Single)
	{
		label << "/threads:" << benchmarkCase.threads;
	}

	std::cout << std::left
			  << std::setw(34) << label.str()
			  << std::setw(10) << benchmarkCase.threads
			  << std::setw(12) << iterations
			  << std::setw(16) << std::fixed << std::setprecision(3) << totalMs
			  << std::setw(16) << averageMs
			  << std::setw(16) << speedup
			  << '\n';

	return averageMs;
}
} // namespace

int main(int argc, char* argv[])
{
	try
	{
		const BenchmarkOptions options = ParseBenchmarkOptions(argc, argv);
		const RgbImage image = PrepareImage(options);
		const std::vector<BenchmarkCase> benchmarkCases = BuildCases(options);

		std::cout << "Image pixels: " << image.PixelCount() << ", bytes: " << image.pixels.size() << '\n';
		PrintHeader();

		double baselineMs = 0.0;
		for (const BenchmarkCase& benchmarkCase : benchmarkCases)
		{
			const double averageMs = RunBenchmarkCase(benchmarkCase, image, options, baselineMs);
			if (benchmarkCase.mode == BenchmarkCase::Mode::Single)
			{
				baselineMs = averageMs;
			}
		}
	}
	catch (const std::exception& exception)
	{
		std::cerr << exception.what() << '\n';
		return EXIT_FAILURE;
	}

	return 0;
}