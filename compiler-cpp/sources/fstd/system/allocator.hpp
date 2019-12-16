#pragma once

#include <stdint.h>

// This module contains basic memory manipulation
// It is not thread-safe.
//
// Allocated memory isn't initialized

namespace fstd
{
	namespace system
	{
		void*	allocate(size_t size);
		void*	reallocate(void* address, size_t size);
		void	free(void* address);

		void	memory_copy(void* destination, void* source, size_t size);
		void	memory_move(void* destination, void* source, size_t size);
		void	fill_memory(void* destination, size_t size, uint8_t value);
		void	zero_memory(void* destination, size_t size);
	}
}