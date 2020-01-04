#include "allocator.hpp"

#include <fstd/platform.hpp>

#if defined(FSTD_OS_WINDOWS)
#	include <Windows.h>
#endif

// Need we use VirtualAlloc instead of HeapAlloc, to support allocation of bigger buffers?

namespace fstd
{
	namespace system
	{
		void* allocate(size_t size)
		{
#if defined(FSTD_OS_WINDOWS)
			return HeapAlloc(GetProcessHeap(), 0, size);
#else
#	error
#endif
		}

		void* reallocate(void* address, size_t size)
		{
#if defined(FSTD_OS_WINDOWS)
			if (address == nullptr) {
				return HeapAlloc(GetProcessHeap(), 0, size);
			}
			else {
				return HeapReAlloc(GetProcessHeap(), 0, address, size);
			}
#else
#	error
#endif
		}

		void free(void* address)
		{
			if (address == nullptr) {
				return;
			}
#if defined(FSTD_OS_WINDOWS)
			HeapFree(GetProcessHeap(), 0, address);
#else
#	error
#endif
		}

		void	memory_copy(void* destination, const void* source, size_t size)
		{
#if defined(FSTD_OS_WINDOWS)
			CopyMemory(destination, source, size);
#else
#	error
#endif
		}

		void	memory_move(void* destination, const void* source, size_t size)
		{
#if defined(FSTD_OS_WINDOWS)
			MoveMemory(destination, source, size);
#else
#	error
#endif
		}

		void	fill_memory(void* destination, size_t size, uint8_t value)
		{
#if defined(FSTD_OS_WINDOWS)
			FillMemory(destination, size, value);
#else
#	error
#endif
		}

		void	zero_memory(void* destination, size_t size)
		{
#if defined(FSTD_OS_WINDOWS)
			SecureZeroMemory(destination, size);
#else
#	error
#endif
		}
	}
}
