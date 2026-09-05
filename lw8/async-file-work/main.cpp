#include "AsyncFile.h"
#include "Dispatcher.h"
#include "Task.h"

#include <iostream>

Task AsyncCopyFile(Dispatcher& dispatcher, const std::string from, const std::string to)
{
	try
	{
		const AsyncFile input = co_await AsyncOpenFile(dispatcher, from, OpenMode::Read);
		const AsyncFile output = co_await AsyncOpenFile(dispatcher, to, OpenMode::Write);

		std::vector<char> buffer(1024);
		ssize_t bytesRead;

		while ((bytesRead = co_await input.ReadAsync(dispatcher, buffer.data(), buffer.size())) > 0)
		{
			co_await output.AsyncWrite(dispatcher, buffer.data(), bytesRead);
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Copy error: " << e.what() << std::endl;
	}
}

Task AsyncCopyTwoFiles(Dispatcher& dispatcher)
{
	std::cout << "Start copying..." << std::endl;
	auto t1 = AsyncCopyFile(dispatcher, "file1.txt", "file1_out.txt");
	auto t2 = AsyncCopyFile(dispatcher, "file2.txt", "file2_out.txt");
	co_await t1;
	co_await t2;
	std::cout << "All copies done.\n";
}

int main()
{
	constexpr int threadsCount = 4;
	Dispatcher dispatcher(threadsCount);
	const auto mainTask = AsyncCopyTwoFiles(dispatcher);

	while (!mainTask.handle.done())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	return 0;
}