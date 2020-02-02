#include <fstd/core/assert.hpp>

#include <fstd/language/types.hpp>

#include <fstd/platform.hpp>

#if defined(_WIN32)
#   include <windows.h>
#elif defined(__unix__) || defined(__linux) || defined(__linux__) || defined(__ANDROID__) || defined(__EPOC32__) || defined(__QNX__)
#   include <time.h>
#elif defined(__APPLE__)
#   include <sys/time.h>
#endif

namespace fstd
{
	namespace system
	{
		inline uint64_t get_time_in_nanoseconds()
		{
#if defined(FSTD_OS_WINDOWS)
			LARGE_INTEGER freq;
			LARGE_INTEGER count;
			QueryPerformanceCounter(&count);
			QueryPerformanceFrequency(&freq);
			fstd::core::Assert(freq.LowPart != 0 || freq.HighPart != 0);

			// This code overflow in nanoseconds but works in microseconds
			// And is certainly more efficient and accurate than passing by a
			// conversion into double
			//
			// Flamaros - 05 january 2020

			//if (count.QuadPart < MAXLONGLONG / 1'000'000'000) {
			//	assert(freq.QuadPart != 0);
			//	return count.QuadPart * 1'000'000'000 / freq.QuadPart;
			//	
			//}
			//else {
			//	assert(freq.QuadPart >= 1'000'000'000);
			//	return count.QuadPart / (freq.QuadPart / 1'000'000'000);
			//}

			double nanoseconds_per_count = 1.0e9 / (double)freq.QuadPart;
			return (uint64_t)(count.QuadPart * nanoseconds_per_count);

#elif defined(FSTD_OS_POSIX_COMPATIBLE)
			struct timespec currTime;
			clock_gettime(CLOCK_MONOTONIC, &currTime);
			return (uint64_t)currTime.tv_sec * 1000000 + ((uint64_t)currTime.tv_nsec / 1000);

#elif defined(FSTD_OS_SYMBIAN)
			struct timespec currTime;
			/* Symbian supports only realtime clock for clock_gettime. */
			clock_gettime(CLOCK_REALTIME, &currTime);
			return (uint64_t)currTime.tv_sec * 1000000 + ((uint64_t)currTime.tv_nsec / 1000);

#elif defined(FSTD_OS_MACOS)
			struct timeval currTime;
			gettimeofday(&currTime, NULL);
			return (uint64_t)currTime.tv_sec * 1000000 + (uint64_t)currTime.tv_usec;
#else
#	error "Not implemented for target OS"
#endif
		}
	}
}
