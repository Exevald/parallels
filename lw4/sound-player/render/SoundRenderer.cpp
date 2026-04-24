#include "render/SoundRenderer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numbers>

namespace
{
struct Voice
{
	float frequency = 0.f;
	Waveform waveform = Waveform::Sine;
	float startTime = 0.f;
	float releaseStartTime = 0.f;
	float endTime = 0.f;
	bool hasRelease = false;
};

float EvaluateWaveform(const Waveform waveform, const float frequency, const float elapsedSeconds)
{
	const float cycles = frequency * elapsedSeconds;
	const float phase = 2.f * std::numbers::pi_v<float> * cycles;
	const float fraction = cycles - std::floor(cycles);

	switch (waveform)
	{
	case Waveform::Sine:
		return std::sin(phase);
	case Waveform::Pulse:
		return std::sin(phase) >= 0.f ? 1.f : -1.f;
	case Waveform::SawDown:
		return 1.f - 2.f * fraction;
	case Waveform::SawUp:
		return 2.f * fraction - 1.f;
	}

	return 0.f;
}

void ReleaseChannelVoices(std::vector<Voice>& voices,
	std::vector<std::size_t>& activeVoiceIndices,
	const float currentTime,
	const float rowDurationSeconds)
{
	for (const std::size_t voiceIndex : activeVoiceIndices)
	{
		if (Voice& voice = voices[voiceIndex]; !voice.hasRelease)
		{
			voice.releaseStartTime = currentTime;
			voice.endTime = currentTime + rowDurationSeconds;
			voice.hasRelease = true;
		}
	}

	activeVoiceIndices.clear();
}
} // namespace

RenderedComposition soundRenderer::Render(const Composition& composition)
{
	std::vector<Voice> voices;
	std::vector<std::vector<std::size_t>> activeVoiceIndices(MAX_MUSIC_CHANNELS);
	float currentTime = 0.f;

	for (const auto& [channels] : composition.rows)
	{
		for (std::size_t channelIndex = 0; channelIndex < channels.size(); ++channelIndex)
		{
			const auto& [releaseActiveNotes, notes] = channels[channelIndex];
			if (releaseActiveNotes)
			{
				ReleaseChannelVoices(
					voices,
					activeVoiceIndices[channelIndex],
					currentTime,
					composition.rowDurationSeconds);
			}

			for (const NoteEvent& note : notes)
			{
				Voice voice;
				voice.frequency = note.frequency;
				voice.waveform = note.waveform;
				voice.startTime = currentTime;
				if (note.releaseImmediately)
				{
					voice.releaseStartTime = currentTime;
					voice.endTime = currentTime + composition.rowDurationSeconds;
					voice.hasRelease = true;
				}

				voices.push_back(voice);
				if (!note.releaseImmediately)
				{
					activeVoiceIndices[channelIndex].push_back(voices.size() - 1);
				}
			}
		}

		currentTime += composition.rowDurationSeconds;
	}

	for (auto& channelVoices : activeVoiceIndices)
	{
		ReleaseChannelVoices(voices, channelVoices, currentTime, composition.rowDurationSeconds);
	}

	const float totalDuration = currentTime + composition.rowDurationSeconds;
	const auto sampleCount = static_cast<std::size_t>(std::ceil(totalDuration * static_cast<float>(SAMPLE_RATE)));
	std::vector mixed(sampleCount, 0.f);

	for (const Voice& voice : voices)
	{
		const auto startSample = std::max(
			0.f,
			std::floor(voice.startTime * static_cast<float>(SAMPLE_RATE)));
		const auto endSample = std::min(
			static_cast<float>(sampleCount),
			std::ceil(voice.endTime * static_cast<float>(SAMPLE_RATE)));

		for (std::size_t sampleIndex = static_cast<long>(startSample);
			static_cast<float>(sampleIndex) < endSample; ++sampleIndex)
		{
			const float time = static_cast<float>(sampleIndex) / static_cast<float>(SAMPLE_RATE);
			float amplitude = 1.f;
			if (voice.hasRelease && time >= voice.releaseStartTime)
			{
				amplitude = 1.f - (time - voice.releaseStartTime) / composition.rowDurationSeconds;
				amplitude = std::max(0.f, amplitude);
			}

			mixed[sampleIndex] += EvaluateWaveform(
									  voice.waveform,
									  voice.frequency,
									  time - voice.startTime)
				* amplitude;
		}
	}

	float peak = 0.f;
	for (const float sample : mixed)
	{
		peak = std::max(peak, std::abs(sample));
	}
	if (peak < 1e-6f)
	{
		peak = 1.f;
	}

	const float normalization = 0.85f / peak;
	std::vector<std::int16_t> samples;
	samples.reserve(mixed.size());
	for (const float sample : mixed)
	{
		const float normalized = std::clamp(sample * normalization, -1.f, 1.f);
		samples.push_back(static_cast<std::int16_t>(
			normalized * static_cast<float>(std::numeric_limits<std::int16_t>::max())));
	}

	return {
		.samples = std::move(samples),
		.durationSeconds = totalDuration,
	};
}
