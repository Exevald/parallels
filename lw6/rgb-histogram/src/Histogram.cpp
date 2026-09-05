#include "Histogram.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <stdexcept>
#include <thread>
#include <vector>

namespace
{

constexpr size_t binsCount = 256;
constexpr size_t channelsCount = 3;
constexpr std::memory_order atomicOrder = std::memory_order_seq_cst;

struct PixelRange
{
	size_t begin = 0;
	size_t end = 0;
};

struct LocalHistogram
{
	std::array<uint32_t, binsCount * channelsCount> bins{};
};

[[nodiscard]] std::vector<PixelRange> SplitWork(const size_t pixelCount, const size_t threadCount)
{
	const size_t actualThreads = ResolveThreadCount(threadCount, pixelCount);
	std::vector<PixelRange> ranges(actualThreads);

	const size_t baseChunk = pixelCount / actualThreads;
	const size_t remainder = pixelCount % actualThreads;

	size_t begin = 0;
	for (size_t index = 0; index < actualThreads; ++index)
	{
		const size_t extra = index < remainder ? 1 : 0;
		const size_t end = begin + baseChunk + extra;
		ranges[index] = PixelRange{ begin, end };
		begin = end;
	}

	return ranges;
}

[[nodiscard]] size_t GetAtomicIndex(const AtomicLayout layout, const uint8_t value, size_t channel)
{
	if (layout == AtomicLayout::InterleavedRgb)
	{
		return static_cast<size_t>(value) * channelsCount + channel;
	}
	return channel * binsCount + static_cast<size_t>(value);
}

[[nodiscard]] HistogramCounts CollectAtomicCounts(
	const std::vector<std::atomic_uint32_t>& counters,
	const AtomicLayout layout)
{
	HistogramCounts counts;
	for (size_t value = 0; value < binsCount; ++value)
	{
		counts.red[value]
			= counters[GetAtomicIndex(layout, static_cast<uint8_t>(value), 0)].load(atomicOrder);
		counts.green[value]
			= counters[GetAtomicIndex(layout, static_cast<uint8_t>(value), 1)].load(atomicOrder);
		counts.blue[value]
			= counters[GetAtomicIndex(layout, static_cast<uint8_t>(value), 2)].load(atomicOrder);
	}
	return counts;
}

void ValidateImage(const RgbImage& image)
{
	if (const size_t expectedSize = image.PixelCount() * channelsCount;
		image.PixelCount() > 0 && image.pixels.size() != expectedSize)
	{
		throw std::invalid_argument("RGB image buffer size does not match width * height * 3");
	}
}

void AccumulateRange(HistogramCounts& counts, const RgbImage& image, PixelRange range)
{
	for (size_t pixelIndex = range.begin; pixelIndex < range.end; ++pixelIndex)
	{
		const size_t offset = pixelIndex * channelsCount;
		++counts.red[image.pixels[offset + 0]];
		++counts.green[image.pixels[offset + 1]];
		++counts.blue[image.pixels[offset + 2]];
	}
}
} // namespace

size_t ResolveThreadCount(const size_t requestedThreads, size_t pixelCount)
{
	if (pixelCount == 0)
	{
		return 1;
	}

	size_t threadCount = requestedThreads;
	if (threadCount == 0)
	{
		threadCount = std::thread::hardware_concurrency();
		if (threadCount == 0)
		{
			threadCount = 1;
		}
	}

	return std::min(threadCount, pixelCount);
}

HistogramCounts BuildHistogramSingleThreadCounts(const RgbImage& image)
{
	ValidateImage(image);

	HistogramCounts counts;
	AccumulateRange(counts, image, PixelRange{ 0, image.PixelCount() });
	return counts;
}

HistogramResult NormalizeHistogram(const HistogramCounts& counts, const size_t pixelCount)
{
	HistogramResult result;
	if (pixelCount == 0)
	{
		return result;
	}

	const float scale = 1.0f / static_cast<float>(pixelCount);
	for (size_t value = 0; value < binsCount; ++value)
	{
		result.red[value] = static_cast<float>(counts.red[value]) * scale;
		result.green[value] = static_cast<float>(counts.green[value]) * scale;
		result.blue[value] = static_cast<float>(counts.blue[value]) * scale;
	}

	return result;
}

HistogramResult BuildHistogramSingleThread(const RgbImage& image)
{
	return NormalizeHistogram(BuildHistogramSingleThreadCounts(image), image.PixelCount());
}

HistogramResult BuildHistogramAtomic(
	const RgbImage& image,
	const size_t threadCount,
	const AtomicLayout layout)
{
	ValidateImage(image);

	std::vector<std::atomic_uint32_t> counters(binsCount * channelsCount);
	for (std::atomic_uint32_t& counter : counters)
	{
		counter.store(0, atomicOrder);
	}

	{
		const std::vector<PixelRange> ranges = SplitWork(image.PixelCount(), threadCount);
		std::vector<std::jthread> workers;
		workers.reserve(ranges.size());

		for (const PixelRange& range : ranges)
		{
			workers.emplace_back([&, range] {
				for (size_t pixelIndex = range.begin; pixelIndex < range.end; ++pixelIndex)
				{
					const size_t offset = pixelIndex * channelsCount;
					counters[GetAtomicIndex(layout, image.pixels[offset + 0], 0)]
						.fetch_add(1, atomicOrder);
					counters[GetAtomicIndex(layout, image.pixels[offset + 1], 1)]
						.fetch_add(1, atomicOrder);
					counters[GetAtomicIndex(layout, image.pixels[offset + 2], 2)]
						.fetch_add(1, atomicOrder);
				}
			});
		}
	}

	return NormalizeHistogram(CollectAtomicCounts(counters, layout), image.PixelCount());
}

HistogramResult BuildHistogramLocal(const RgbImage& image, const size_t threadCount)
{
	ValidateImage(image);

	const std::vector<PixelRange> ranges = SplitWork(image.PixelCount(), threadCount);
	std::vector<LocalHistogram> localHistograms(ranges.size());
	{
		std::vector<std::jthread> workers;
		workers.reserve(ranges.size());

		for (size_t workerIndex = 0; workerIndex < ranges.size(); ++workerIndex)
		{
			workers.emplace_back([&, workerIndex] {
				const PixelRange range = ranges[workerIndex];
				auto& bins = localHistograms[workerIndex].bins;
				for (size_t pixelIndex = range.begin; pixelIndex < range.end; ++pixelIndex)
				{
					const size_t offset = pixelIndex * channelsCount;
					++bins[static_cast<size_t>(image.pixels[offset + 0])];
					++bins[binsCount + static_cast<size_t>(image.pixels[offset + 1])];
					++bins[2 * binsCount + static_cast<size_t>(image.pixels[offset + 2])];
				}
			});
		}
	}

	HistogramCounts merged;
	for (const LocalHistogram& histogram : localHistograms)
	{
		for (size_t value = 0; value < binsCount; ++value)
		{
			merged.red[value] += histogram.bins[value];
			merged.green[value] += histogram.bins[binsCount + value];
			merged.blue[value] += histogram.bins[2 * binsCount + value];
		}
	}

	return NormalizeHistogram(merged, image.PixelCount());
}

bool HistogramsAlmostEqual(const HistogramResult& lhs, const HistogramResult& rhs, float epsilon)
{
	for (size_t value = 0; value < binsCount; ++value)
	{
		if (std::fabs(lhs.red[value] - rhs.red[value]) > epsilon)
		{
			return false;
		}
		if (std::fabs(lhs.green[value] - rhs.green[value]) > epsilon)
		{
			return false;
		}
		if (std::fabs(lhs.blue[value] - rhs.blue[value]) > epsilon)
		{
			return false;
		}
	}
	return true;
}