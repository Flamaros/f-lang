#include "allocator.hpp"

#include <fstd/platform.hpp>

#if defined(PLATFORM_WINDOWS)
#	include <Windows.h>
#endif

// Need we use VirtualAlloc instead of HeapAlloc, to support allocation of bigger buffers?

namespace fstd
{
	namespace system
	{
		void* allocate(size_t size)
		{
#if defined(PLATFORM_WINDOWS)
			return HeapAlloc(GetProcessHeap(), 0, size);
#else
#	error
#endif
		}

		void* reallocate(void* address, size_t size)
		{
#if defined(PLATFORM_WINDOWS)
			return HeapReAlloc(GetProcessHeap(), 0, address, size);
#else
#	error
#endif
		}

		void free(void* address)
		{
			if (address == nullptr) {
				return;
			}
#if defined(PLATFORM_WINDOWS)
			HeapFree(GetProcessHeap(), 0, address);
#else
#	error
#endif
		}

		void	memory_copy(void* destination, void* source, size_t size)
		{
#if defined(PLATFORM_WINDOWS)
			CopyMemory(destination, source, size);
#else
#	error
#endif
		}

		void	memory_move(void* destination, void* source, size_t size)
		{
#if defined(PLATFORM_WINDOWS)
			MoveMemory(destination, source, size);
#else
#	error
#endif
		}

		void	memory_fill(void* destination, size_t size, uint8_t value)
		{
#if defined(PLATFORM_WINDOWS)
			FillMemory(destination, size, value);
#else
#	error
#endif
		}

		void	memory_zero(void* destination, size_t size)
		{
#if defined(PLATFORM_WINDOWS)
			SecureZeroMemory(destination, size);
#else
#	error
#endif
		}


	}
}
