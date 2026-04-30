#include "AtomicMax.h"
#include "Timer.h"

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace
{
enum class Workload
{
	Monotonic,
	Contention,
};

struct Options
{
	int maxThreads = 30;
	int iterationsPerThread = 200000;
	std::string csvPath;
};

struct ResultRow
{
	std::string implementation;
	int threads = 0;
	int iterationsPerThread = 0;
	std::string workload;
	double totalMs = 0.0;
	double avgNsPerUpdate = 0.0;
};

Options ParseOptions(int argc, char* argv[])
{
	Options options;
	for (int index = 1; index < argc; ++index)
	{
		const std::string_view argument(argv[index]);
		if (argument.rfind("--max-threads=", 0) == 0)
		{
			options.maxThreads = std::stoi(std::string(argument.substr(14)));
			continue;
		}
		if (argument.rfind("--iterations=", 0) == 0)
		{
			options.iterationsPerThread = std::stoi(std::string(argument.substr(13)));
			continue;
		}
		if (argument.rfind("--csv=", 0) == 0)
		{
			options.csvPath = std::string(argument.substr(6));
			continue;
		}
		throw std::invalid_argument("Unknown argument: " + std::string(argument));
	}

	if (options.maxThreads < 1)
	{
		throw std::invalid_argument("--max-threads must be positive");
	}
	if (options.iterationsPerThread < 1)
	{
		throw std::invalid_argument("--iterations must be positive");
	}

	return options;
}

template <typename AtomicMaxType>
ResultRow RunCase(
	const std::string& implementation,
	const int threads,
	const int iterationsPerThread,
	const Workload workload)
{
	AtomicMaxType atomicMax(0);
	std::vector<std::thread> workers;
	workers.reserve(static_cast<size_t>(threads));

	timer::Stopwatch stopwatch;
	for (int threadIndex = 0; threadIndex < threads; ++threadIndex)
	{
		workers.emplace_back([&, threadIndex] {
			for (int iteration = 0; iteration < iterationsPerThread; ++iteration)
			{
				int value = 0;
				if (workload == Workload::Monotonic)
				{
					value = threadIndex * iterationsPerThread + iteration;
				}
				else
				{
					value = (iteration * 13 + threadIndex * 7) % (iterationsPerThread * 2);
				}
				atomicMax.Update(value);
			}
		});
	}
	for (auto& worker : workers)
	{
		worker.join();
	}
	stopwatch.Stop();

	const auto expectedLabel = workload == Workload::Monotonic ? "monotonic" : "contention";
	const double totalUpdates = static_cast<double>(threads) * static_cast<double>(iterationsPerThread);
	const double totalMs = stopwatch.ElapsedSeconds() * 1000.0;
	const double avgNsPerUpdate = stopwatch.ElapsedSeconds() * 1'000'000'000.0 / totalUpdates;

	return ResultRow{
		implementation,
		threads,
		iterationsPerThread,
		expectedLabel,
		totalMs,
		avgNsPerUpdate,
	};
}

void PrintTableHeader()
{
	std::cout << std::left
			  << std::setw(18) << "impl"
			  << std::setw(10) << "threads"
			  << std::setw(14) << "iterations"
			  << std::setw(14) << "workload"
			  << std::setw(14) << "total_ms"
			  << std::setw(18) << "avg_ns_update"
			  << '\n';
}

void PrintRow(const ResultRow& row)
{
	std::cout << std::left
			  << std::setw(18) << row.implementation
			  << std::setw(10) << row.threads
			  << std::setw(14) << row.iterationsPerThread
			  << std::setw(14) << row.workload
			  << std::setw(14) << std::fixed << std::setprecision(3) << row.totalMs
			  << std::setw(18) << row.avgNsPerUpdate
			  << '\n';
}

void WriteCsv(const std::string& path, const std::vector<ResultRow>& rows)
{
	if (path.empty())
	{
		return;
	}

	std::ofstream output(path);
	if (!output.is_open())
	{
		throw std::runtime_error("Failed to open CSV file: " + path);
	}

	output << "impl,threads,iterations,workload,total_ms,avg_ns_update\n";
	for (const auto& row : rows)
	{
		output << row.implementation << ','
			   << row.threads << ','
			   << row.iterationsPerThread << ','
			   << row.workload << ','
			   << row.totalMs << ','
			   << row.avgNsPerUpdate << '\n';
	}

	if (!output.good())
	{
		throw std::runtime_error("Failed to write CSV file: " + path);
	}
}
} // namespace

int main(const int argc, char* argv[])
{
	try
	{
		const Options options = ParseOptions(argc, argv);
		std::vector<ResultRow> rows;
		rows.reserve(static_cast<size_t>(options.maxThreads) * 4);

		PrintTableHeader();
		for (int threads = 1; threads <= options.maxThreads; ++threads)
		{
			for (const Workload workload : { Workload::Monotonic, Workload::Contention })
			{
				const auto atomicRow = RunCase<AtomicMax<int>>(
					"atomic",
					threads,
					options.iterationsPerThread,
					workload);
				const auto lockRow = RunCase<AtomicMaxWithLock<int>>(
					"lock",
					threads,
					options.iterationsPerThread,
					workload);
				PrintRow(atomicRow);
				PrintRow(lockRow);
				rows.push_back(atomicRow);
				rows.push_back(lockRow);
			}
		}

		WriteCsv(options.csvPath, rows);
		return EXIT_SUCCESS;
	}
	catch (const std::exception& exception)
	{
		std::cerr << exception.what() << '\n';
		return EXIT_FAILURE;
	}
}