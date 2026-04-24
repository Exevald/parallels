#include "io/AudioPlayer.h"
#include "io/WavWriter.h"
#include "parser/MelodyParser.h"
#include "render/SoundRenderer.h"
#include "ui/WaveformVisualizer.h"

#include <iostream>
#include <thread>

// сделать realtime рендеринг аудио
int main(const int argc, char* argv[])
{
	if (argc != 2)
	{
		std::cerr << "Usage: sound-player <melody-file>\n";
		return 1;
	}

	try
	{
		const Composition composition = melodyParser::ParseFile(argv[1]);
		const RenderedComposition renderedComposition = soundRenderer::Render(composition);
		const std::string wavPath = "/tmp/sound-player-output.wav";
		wavWriter::Save(wavPath, renderedComposition.samples);

		// синхронизация
		std::jthread visualizationThread([&renderedComposition] {
			waveformVisualizer::Show(renderedComposition);
		});

		if (const int playerResult = audioPlayer::PlayFile(wavPath);
			playerResult != 0)
		{
			std::cerr << "Warning: afplay exited with code " << playerResult << '\n';
		}
		return 0;
	}
	catch (const std::exception& exception)
	{
		std::cerr << "Error: " << exception.what() << '\n';
		return 1;
	}
}
