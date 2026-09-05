#pragma once

#include "ImageLoader.h"

#include <array>
#include <cstddef>

enum class AtomicLayout
{
	InterleavedRgb,
	ChannelBlocks,
};

struct HistogramResult
{
	std::array<float, 256> red{};
	std::array<float, 256> green{};
	std::array<float, 256> blue{};
};

struct HistogramCounts
{
	std::array<uint32_t, 256> red{};
	std::array<uint32_t, 256> green{};
	std::array<uint32_t, 256> blue{};
};

[[nodiscard]] size_t ResolveThreadCount(size_t requestedThreads, size_t pixelCount);
[[nodiscard]] HistogramCounts BuildHistogramSingleThreadCounts(const RgbImage& image);
[[nodiscard]] HistogramResult BuildHistogramSingleThread(const RgbImage& image);
[[nodiscard]] HistogramResult BuildHistogramAtomic(const RgbImage& image, size_t threadCount, AtomicLayout layout);
[[nodiscard]] HistogramResult BuildHistogramLocal(const RgbImage& image, size_t threadCount);
[[nodiscard]] HistogramResult NormalizeHistogram(const HistogramCounts& counts, size_t pixelCount);
[[nodiscard]] bool HistogramsAlmostEqual(const HistogramResult& lhs, const HistogramResult& rhs, float epsilon);
