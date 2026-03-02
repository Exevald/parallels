#pragma once

#include <cmath>

constexpr double GAMMA = 2.2;
constexpr double INV_GAMMA = 1.0 / GAMMA;

inline double ToLinear(int value)
{
	float normalized = static_cast<float>(value) / 255.0f;
	return std::pow(normalized, GAMMA);
}

inline int ToGamma(double value)
{
	if (value < 0.0f)
	{
		value = 0.0f;
	}
	if (value > 1.0f)
	{
		value = 1.0f;
	}
	return static_cast<int>(std::round(std::pow(value, INV_GAMMA) * 255.0f));
}
