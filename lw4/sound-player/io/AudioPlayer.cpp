#include "io/AudioPlayer.h"

#include <cstdlib>
#include <stdexcept>

int audioPlayer::PlayFile(const std::string& wavPath)
{
	const std::string command = "/usr/bin/afplay \"" + wavPath + "\"";
	const int result = std::system(command.c_str());
	if (result == -1)
	{
		throw std::runtime_error("Failed to run afplay");
	}

	return result;
}
