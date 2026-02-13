#include "ArgsParser.h"
#include "Decompressor.h"
#include "Extractor.h"
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

	if (!extractor::ExtractFilesFromArchive(args->archiveName, args->outputFolder))
	{
		std::cerr << "Failed to extract files from archive\n";
		return EXIT_FAILURE;
	}

	auto compressedFiles = extractor::GetExtractedFiles(args->outputFolder);
	std::cout << "Extracted " << compressedFiles.size() << " file(s) to '" << args->outputFolder << "'\n";

	bool success;
	if (args->mode == argsParser::Mode::Sequential)
	{
		success = decompressor::DecompressSequentially(compressedFiles);
	}
	else
	{
		success = decompressor::DecompressInParallel(compressedFiles, args->numProcesses);
	}

	if (!success)
	{
		std::cerr << "Failed to decompress files\n";
		return EXIT_FAILURE;
	}

	totalTimer.Stop();

	printf("Total time: %.6f seconds\n", totalTimer.ElapsedSeconds());

	return EXIT_SUCCESS;
}