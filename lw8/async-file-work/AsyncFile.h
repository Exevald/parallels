#pragma once

#include "Dispatcher.h"

#include <coroutine>
#include <sys/fcntl.h>
#include <unistd.h>
#include <utility>

enum class OpenMode
{
	Read,
	Write
};

class AsyncFile
{
	int fd = -1;

public:
	explicit AsyncFile(int f)
		: fd(f)
	{
	}
	~AsyncFile()
	{
		if (fd != -1)
		{
			close(fd);
		}
	}
	AsyncFile(AsyncFile&& other) noexcept
		: fd(std::exchange(other.fd, -1))
	{
	}
	AsyncFile(const AsyncFile&) = delete;

	struct ReadAwaiter
	{
		int fd;
		Dispatcher& dispatcher;
		void* buf;
		size_t size;
		ssize_t result = 0;
		int error = 0;

		bool await_ready() { return false; }
		void await_suspend(std::coroutine_handle<> handle)
		{
			dispatcher.Post([this, handle] {
				result = read(fd, buf, size);
				if (result == -1)
				{
					error = errno;
				}
				handle.resume();
			});
		}
		ssize_t await_resume()
		{
			if (result == -1)
			{
				throw std::system_error(error, std::generic_category(), "Read failed");
			}
			return result;
		}
	};

	struct WriteAwaiter
	{
		int fd;
		Dispatcher& dispatcher;
		const void* buf;
		size_t size;
		ssize_t result = 0;
		int error = 0;

		bool await_ready() { return false; }
		void await_suspend(std::coroutine_handle<> h)
		{
			dispatcher.Post([this, h] {
				result = ::write(fd, buf, size);
				if (result == -1)
				{
					error = errno;
				}
				h.resume();
			});
		}
		void await_resume()
		{
			if (result == -1)
			{
				throw std::system_error(error, std::generic_category(), "Write failed");
			}
		}
	};

	auto ReadAsync(Dispatcher& dispatcher, void* buffer, const size_t size) const
	{
		return ReadAwaiter{ fd, dispatcher, buffer, size };
	}
	auto AsyncWrite(Dispatcher& dispatcher, const void* buffer, const size_t s) const
	{
		return WriteAwaiter{ fd, dispatcher, buffer, s };
	}
};

struct OpenAwaiter
{
	Dispatcher& dispatcher;
	std::string path;
	OpenMode mode;
	int resultFd = -1;
	int error = 0;

	bool await_ready() { return false; }
	void await_suspend(std::coroutine_handle<> handle)
	{
		dispatcher.Post([this, handle] {
			const int flags = (mode == OpenMode::Read) ? O_RDONLY : (O_WRONLY | O_CREAT | O_TRUNC);
			resultFd = open(path.c_str(), flags, 0644);
			if (resultFd == -1)
			{
				error = errno;
			}
			handle.resume();
		});
	}
	AsyncFile await_resume()
	{
		if (resultFd == -1)
		{
			throw std::system_error(error, std::generic_category(), "Open failed: " + path);
		}
		return AsyncFile{ resultFd };
	}
};

inline auto AsyncOpenFile(Dispatcher& d, std::string path, const OpenMode mode)
{
	return OpenAwaiter{ d, std::move(path), mode };
}