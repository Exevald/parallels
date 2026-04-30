#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
#define BENCHMARK_EXPORT
#else
#define BENCHMARK_EXPORT __attribute__((visibility("default")))
#endif
