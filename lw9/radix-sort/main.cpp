#include "GpuRadixSort.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <oneapi/dpl/algorithm>
#include <oneapi/dpl/execution>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

constexpr std::size_t DEFAULT_ELEMENT_COUNT = 100'000'000;

std::size_t ParseElementCount(const int argc, char* argv[])
{
	if (argc > 2)
	{
		throw std::invalid_argument("Usage: radix-sort [element-count]");
	}
	if (argc == 1)
	{
		return DEFAULT_ELEMENT_COUNT;
	}

	std::size_t parsed = 0;
	const std::string argument = argv[1];
	const unsigned long long value = std::stoull(argument, &parsed);
	if (parsed != argument.size() || value > std::numeric_limits<std::size_t>::max())
	{
		throw std::invalid_argument("Invalid element count: " + argument);
	}
	return value;
}

std::vector<std::int32_t> GenerateValues(const std::size_t count)
{
	std::mt19937 generator(42);
	std::uniform_int_distribution distribution(
		std::numeric_limits<std::int32_t>::min(),
		std::numeric_limits<std::int32_t>::max());
	std::vector<std::int32_t> values(count);
	for (std::int32_t& value : values)
	{
		value = distribution(generator);
	}
	return values;
}

float SortOnCpu(std::vector<std::int32_t>& values)
{
	const auto start = std::chrono::steady_clock::now();
	std::sort(dpl::execution::seq, values.begin(), values.end());
	return std::chrono::duration<float, std::milli>(
		std::chrono::steady_clock::now() - start)
		.count();
}

} // namespace

int main(const int argc, char* argv[])
{
	try
	{
		const std::size_t count = ParseElementCount(argc, argv);
		const std::vector<std::int32_t> source = GenerateValues(count);
		std::vector<std::int32_t> cpuValues = source;
		std::vector<std::int32_t> gpuValues = source;

		GpuRadixSort gpuSorter;
		const float cpuTimeMs = SortOnCpu(cpuValues);
		const float gpuTimeMs = gpuSorter.Sort(gpuValues);
		const bool correct = cpuValues == gpuValues;

		std::cout << std::fixed << std::setprecision(3)
				  << "Elements: " << count << '\n'
				  << "CPU parallel std::sort: " << cpuTimeMs << " ms\n"
				  << "GPU radix sort: " << gpuTimeMs << " ms\n";
		if (gpuTimeMs > 0.0f)
		{
			std::cout << "Speedup: " << cpuTimeMs / gpuTimeMs << "x\n";
		}
		std::cout << "Result: " << (correct ? "correct" : "incorrect") << '\n';
		return correct ? EXIT_SUCCESS : EXIT_FAILURE;
	}
	catch (const std::exception& error)
	{
		std::cerr << "Error: " << error.what() << '\n';
		return EXIT_FAILURE;
	}
}
