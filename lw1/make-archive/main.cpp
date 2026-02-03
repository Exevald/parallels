#include "Archiver.h"
#include "ArgsParser.h"
#include "Compressor.h"
#include "TempFileManager.h"
#include "Timer.h"

#include <cstdio>
#include <iostream>

int main(int argc, char* argv[])
{
	auto args = argsParser::ParseArgs(argc, argv);
	if (!args)
	{
		return EXIT_FAILURE;
	}

	timer::Stopwatch totalTimer;

	compressor::CompressionResult compressionResult;
	if (args->mode == argsParser::Mode::Sequential)
	{
		compressionResult = compressor::CompressSequentially(args->inputFiles);
	}
	else
	{
		compressionResult = compressor::CompressInParallel(args->inputFiles, args->numProcesses);
	}

	timer::Stopwatch archivingTimer;
	if (!archiver::CreateTarArchive(args->archiveName, compressionResult.compressedFiles))
	{
		std::cout << "Error: Failed to create tar archive" << std::endl;
		tempFileManager::CleanupTempFiles(compressionResult.tempFiles);

		return EXIT_FAILURE;
	}

	archivingTimer.Stop();
	totalTimer.Stop();

	tempFileManager::CleanupTempFiles(compressionResult.tempFiles);

	printf("Total time: %.6f seconds\n", totalTimer.ElapsedSeconds());
	printf("Sequential part time: %.6f seconds\n", archivingTimer.ElapsedSeconds());

	return EXIT_SUCCESS;
}