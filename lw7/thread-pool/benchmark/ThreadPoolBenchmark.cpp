#include "LockBasedThreadPool.h"
#include "LockFreeThreadPool.h"
#include "Timer.h"

#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>

#include <atomic>
#include <cstdlib>
#include <fstream>
#include <future>
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
	ShortCpu,
	HeavyCpu,
};

struct Options
{
	size_t taskCount = 50000;
	std::string csvPath;
};

struct ResultRow
{
	std::string implementation;
	size_t threads = 0;
	size_t taskCount = 0;
	std::string workload;
	double totalMs = 0.0;
	double avgUsPerTask = 0.0;
};

Options ParseOptions(int argc, char* argv[])
{
	Options options;
	for (int index = 1; index < argc; ++index)
	{
		const std::string_view argument(argv[index]);
		if (argument.rfind("--tasks=", 0) == 0)
		{
			options.taskCount = static_cast<size_t>(std::stoull(std::string(argument.substr(8))));
			continue;
		}
		if (argument.rfind("--csv=", 0) == 0)
		{
			options.csvPath = std::string(argument.substr(6));
			continue;
		}
		throw std::invalid_argument("Unknown argument: " + std::string(argument));
	}

	if (options.taskCount == 0)
	{
		throw std::invalid_argument("--tasks must be positive");
	}

	return options;
}

size_t DetectMaxThreads()
{
	const unsigned int hardwareThreads = std::thread::hardware_concurrency();
	const size_t baseline = hardwareThreads == 0 ? 2u : static_cast<size_t>(hardwareThreads);
	return baseline * 2;
}

std::string ToString(Workload workload)
{
	return workload == Workload::ShortCpu ? "short_cpu" : "heavy_cpu";
}

std::uint64_t ExecuteWorkload(const Workload workload, size_t index)
{
	std::uint64_t value = index + 1;
	const size_t rounds = workload == Workload::ShortCpu ? 32 : 2048;
	for (size_t round = 0; round < rounds; ++round)
	{
		value = value * 1664525u + 1013904223u + round;
		value ^= value >> 13;
	}
	return value;
}

template <typename Pool>
ResultRow RunPoolCase(
	const std::string& implementation,
	size_t threads,
	size_t taskCount,
	Workload workload)
{
	Pool pool(threads);
	std::atomic<std::uint64_t> sink{ 0 };
	std::vector<std::future<void>> futures;
	futures.reserve(taskCount);

	timer::Stopwatch stopwatch;
	for (size_t index = 0; index < taskCount; ++index)
	{
		futures.push_back(pool.Enqueue([&, index] {
			sink.fetch_add(ExecuteWorkload(workload, index), std::memory_order_relaxed);
		}));
	}
	for (auto& future : futures)
	{
		future.get();
	}
	pool.Wait();
	stopwatch.Stop();

	if (sink.load(std::memory_order_relaxed) == 0)
	{
		std::cerr << "Unexpected zero sink value\n";
	}

	const double totalMs = stopwatch.ElapsedSeconds() * 1000.0;
	return ResultRow{
		implementation,
		threads,
		taskCount,
		ToString(workload),
		totalMs,
		stopwatch.ElapsedSeconds() * 1'000'000.0 / static_cast<double>(taskCount),
	};
}

ResultRow RunAsioCase(size_t threads, size_t taskCount, Workload workload)
{
	boost::asio::thread_pool pool(threads);
	std::atomic<std::uint64_t> sink{ 0 };

	timer::Stopwatch stopwatch;
	for (size_t index = 0; index < taskCount; ++index)
	{
		boost::asio::post(pool, [&, index] {
			sink.fetch_add(ExecuteWorkload(workload, index), std::memory_order_relaxed);
		});
	}
	pool.join();
	stopwatch.Stop();

	if (sink.load(std::memory_order_relaxed) == 0)
	{
		std::cerr << "Unexpected zero sink value\n";
	}

	const double totalMs = stopwatch.ElapsedSeconds() * 1000.0;
	return ResultRow{
		"boost_asio",
		threads,
		taskCount,
		ToString(workload),
		totalMs,
		stopwatch.ElapsedSeconds() * 1'000'000.0 / static_cast<double>(taskCount),
	};
}

void PrintHeader()
{
	std::cout << std::left
			  << std::setw(18) << "impl"
			  << std::setw(10) << "threads"
			  << std::setw(12) << "tasks"
			  << std::setw(14) << "workload"
			  << std::setw(14) << "total_ms"
			  << std::setw(18) << "avg_us_per_task"
			  << '\n';
}

void PrintRow(const ResultRow& row)
{
	std::cout << std::left
			  << std::setw(18) << row.implementation
			  << std::setw(10) << row.threads
			  << std::setw(12) << row.taskCount
			  << std::setw(14) << row.workload
			  << std::setw(14) << std::fixed << std::setprecision(3) << row.totalMs
			  << std::setw(18) << row.avgUsPerTask
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

	output << "impl,threads,task_count,workload,total_ms,avg_us_per_task\n";
	for (const auto& row : rows)
	{
		output << row.implementation << ','
			   << row.threads << ','
			   << row.taskCount << ','
			   << row.workload << ','
			   << row.totalMs << ','
			   << row.avgUsPerTask << '\n';
	}

	if (!output.good())
	{
		throw std::runtime_error("Failed to write CSV file: " + path);
	}
}
} // namespace

int main(int argc, char* argv[])
{
	try
	{
		const Options options = ParseOptions(argc, argv);
		const size_t maxThreads = DetectMaxThreads();
		std::vector<ResultRow> rows;
		rows.reserve(maxThreads * 6);

		PrintHeader();
		for (size_t threads = 1; threads <= maxThreads; ++threads)
		{
			for (const Workload workload : { Workload::ShortCpu, Workload::HeavyCpu })
			{
				const auto lockFreeRow = RunPoolCase<LockFreeThreadPool>(
					"lock_free",
					threads,
					options.taskCount,
					workload);
				const auto lockRow = RunPoolCase<LockBasedThreadPool>(
					"lock_based",
					threads,
					options.taskCount,
					workload);
				const auto asioRow = RunAsioCase(threads, options.taskCount, workload);

				PrintRow(lockFreeRow);
				PrintRow(lockRow);
				PrintRow(asioRow);

				rows.push_back(lockFreeRow);
				rows.push_back(lockRow);
				rows.push_back(asioRow);
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