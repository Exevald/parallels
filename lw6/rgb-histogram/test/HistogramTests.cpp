#include "Histogram.h"
#include "ImageLoader.h"

#include <gtest/gtest.h>

namespace
{
float SumChannel(const std::array<float, 256>& histogram)
{
	float sum = 0.0f;
	for (const float value : histogram)
	{
		sum += value;
	}
	return sum;
}
} // namespace

TEST(Histogram, ParallelImplementationsMatchSingleThread)
{
	const RgbImage image = ImageLoader::GenerateRandomImage(257, 129, 12345);
	const HistogramResult reference = BuildHistogramSingleThread(image);

	EXPECT_TRUE(HistogramsAlmostEqual(
		reference,
		BuildHistogramAtomic(image, 4, AtomicLayout::InterleavedRgb),
		1e-6f));
	EXPECT_TRUE(HistogramsAlmostEqual(
		reference,
		BuildHistogramAtomic(image, 4, AtomicLayout::ChannelBlocks),
		1e-6f));
	EXPECT_TRUE(HistogramsAlmostEqual(
		reference,
		BuildHistogramLocal(image, 4), 1e-6f));
}

TEST(Histogram, HistogramSumsAreOneForEveryChannel)
{
	const RgbImage image = ImageLoader::GenerateGradient(64, 32);
	const HistogramResult histogram = BuildHistogramLocal(image, 8);

	EXPECT_NEAR(SumChannel(histogram.red), 1.0f, 1e-6f);
	EXPECT_NEAR(SumChannel(histogram.green), 1.0f, 1e-6f);
	EXPECT_NEAR(SumChannel(histogram.blue), 1.0f, 1e-6f);
}

TEST(Histogram, SolidColorFallsIntoSingleBin)
{
	const RgbImage image = ImageLoader::GenerateSolidColor(100, 50, 10, 20, 30);
	const HistogramResult histogram = BuildHistogramSingleThread(image);

	EXPECT_FLOAT_EQ(histogram.red[10], 1.0f);
	EXPECT_FLOAT_EQ(histogram.green[20], 1.0f);
	EXPECT_FLOAT_EQ(histogram.blue[30], 1.0f);
	EXPECT_FLOAT_EQ(histogram.red[9], 0.0f);
	EXPECT_FLOAT_EQ(histogram.green[19], 0.0f);
	EXPECT_FLOAT_EQ(histogram.blue[29], 0.0f);
}

TEST(Histogram, HandlesMoreThreadsThanPixels)
{
	const RgbImage image = ImageLoader::GenerateRandomImage(3, 1, 77);
	const HistogramResult reference = BuildHistogramSingleThread(image);
	const HistogramResult local = BuildHistogramLocal(image, 64);

	EXPECT_TRUE(HistogramsAlmostEqual(reference, local, 1e-6f));
}