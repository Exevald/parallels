#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

constexpr unsigned SAMPLE_RATE = 44100;
constexpr unsigned CHANNEL_COUNT = 1;
constexpr std::size_t MAX_MUSIC_CHANNELS = 10;

enum class Waveform
{
	Sine,
	Pulse,
	SawDown,
	SawUp,
};

struct NoteEvent
{
	float frequency = 0.f;
	Waveform waveform = Waveform::Sine;
	bool releaseImmediately = false;
};

struct ChannelAction
{
	bool releaseActiveNotes = false;
	std::vector<NoteEvent> notes;
};

struct Row
{
	std::vector<ChannelAction> channels;
};

struct Composition
{
	int tempo = 0;
	float rowDurationSeconds = 0.f;
	std::vector<Row> rows;
};

struct RenderedComposition
{
	std::vector<std::int16_t> samples;
	float durationSeconds = 0.f;
};
