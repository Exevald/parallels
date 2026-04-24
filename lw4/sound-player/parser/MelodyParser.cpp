#include "parser/MelodyParser.h"
#include "common/StringUtils.h"

#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace
{
int LetterToSemitone(const char letter)
{
	static const std::unordered_map<char, int> offsets = {
		{ 'C', 0 },
		{ 'D', 2 },
		{ 'E', 4 },
		{ 'F', 5 },
		{ 'G', 7 },
		{ 'A', 9 },
		{ 'B', 11 },
	};

	const auto it = offsets.find(letter);
	if (it == offsets.end())
	{
		throw std::runtime_error("Unsupported note letter");
	}

	return it->second;
}

float FrequencyFromNote(char letter, bool sharp, int octave)
{
	const int semitone = LetterToSemitone(letter) + (sharp ? 1 : 0);
	const int midi = (octave + 1) * 12 + semitone;
	return 440.f * std::pow(2.f, static_cast<float>(midi - 69) / 12.f);
}

ChannelAction ParseChannelAction(
	const std::string& source,
	std::size_t lineNumber,
	std::size_t channelIndex)
{
	ChannelAction action;
	std::string compact;
	compact.reserve(source.size());
	for (char ch : source)
	{
		if (!std::isspace(static_cast<unsigned char>(ch)))
		{
			compact.push_back(ch);
		}
	}

	std::size_t index = 0;
	while (index < compact.size())
	{
		if (compact[index] == '-')
		{
			action.releaseActiveNotes = true;
			++index;
			continue;
		}

		if (compact[index] < 'A' || compact[index] > 'G')
		{
			std::ostringstream message;
			message << "Invalid token in line " << lineNumber << ", channel " << (channelIndex + 1);
			throw std::runtime_error(message.str());
		}

		const char letter = compact[index++];
		bool sharp = false;
		if (index < compact.size() && compact[index] == '#')
		{
			sharp = true;
			++index;
		}

		if (index >= compact.size() || compact[index] < '0' || compact[index] > '8')
		{
			std::ostringstream message;
			message << "Missing octave in line " << lineNumber << ", channel " << (channelIndex + 1);
			throw std::runtime_error(message.str());
		}

		const int octave = compact[index++] - '0';
		Waveform waveform = Waveform::Sine;
		if (index < compact.size())
		{
			switch (compact[index])
			{
			case 'P':
				waveform = Waveform::Pulse;
				++index;
				break;
			case '\\':
				waveform = Waveform::SawDown;
				++index;
				break;
			case 'W':
				waveform = Waveform::SawUp;
				++index;
				break;
			default:
				break;
			}
		}

		bool releaseImmediately = false;
		if (index < compact.size() && compact[index] == '-')
		{
			releaseImmediately = true;
			++index;
		}

		action.notes.push_back({
			.frequency = FrequencyFromNote(letter, sharp, octave),
			.waveform = waveform,
			.releaseImmediately = releaseImmediately,
		});
	}

	return action;
}
} // namespace

Composition melodyParser::ParseFile(const std::string& path)
{
	std::ifstream input(path);
	if (!input.is_open())
	{
		throw std::runtime_error("Failed to open input file: " + path);
	}

	Composition composition;
	std::string line;
	if (!std::getline(input, line))
	{
		throw std::runtime_error("The melody file is empty");
	}

	const std::string tempoText = Trim(line);
	if (tempoText.empty())
	{
		throw std::runtime_error("The first line must contain the tempo");
	}

	composition.tempo = std::stoi(tempoText);
	if (composition.tempo <= 0)
	{
		throw std::runtime_error("Tempo must be positive");
	}
	composition.rowDurationSeconds = 60.f / static_cast<float>(composition.tempo);

	std::size_t lineNumber = 1;
	while (std::getline(input, line))
	{
		++lineNumber;
		if (Trim(line) == "END")
		{
			break;
		}

		Row row;
		const auto channelTexts = Split(line, '|');
		if (channelTexts.size() > MAX_MUSIC_CHANNELS)
		{
			throw std::runtime_error("Too many independent channels in line " + std::to_string(lineNumber));
		}

		row.channels.reserve(channelTexts.size());
		for (std::size_t channelIndex = 0; channelIndex < channelTexts.size(); ++channelIndex)
		{
			row.channels.push_back(ParseChannelAction(channelTexts[channelIndex], lineNumber, channelIndex));
		}

		composition.rows.push_back(std::move(row));
	}

	return composition;
}
