#pragma once

#include <fstd/language/types.hpp>

// This module contains basic memory manipulation
// It is not thread-safe.
//
// Allocated memory isn't initialized

namespace fstd
{
	namespace system
	{
		void	allocator_initialize();
		void*	allocate(size_t size);
		void*	reallocate(void* address, size_t size);
		void	free(void* address);

		void	memory_copy(void* destination, const void* source, size_t size);
		void	memory_move(void* destination, const void* source, size_t size);
		void	fill_memory(void* destination, size_t size, uint8_t value);
		void	zero_memory(void* destination, size_t size);

		inline bool memory_compare(const void* a, const void* b, size_t size)
		{
			// @TODO @SpeedUp
			// We may want to go into ASM: http://faydoc.tripod.com/cpu/scasb.htm
			// Or at least do the comparaison per block of size_t sizeof,...
			// https://stackoverflow.com/questions/1684898/whats-the-most-efficient-way-to-compare-two-blocks-of-memory-in-the-d-language
			//
            // We also want to do complete unroll when the size is small and known at compile time.
            // https://stackoverflow.com/questions/855895/intrinsic-memcmp
            //
			// Flamaros - 16 february 2020

            uint8_t* lhs = (uint8_t*)a;
            uint8_t* rhs = (uint8_t*)b;

            const uint8_t* endLHS = &lhs[size];
            while (lhs < endLHS) {
                //A good optimiser will keep these values in register
                //and may even be clever enough to just retest the flags
                //before incrementing the pointers iff we loop again.
                //gcc -O3 did the optimisation very well.
                if (*lhs > *rhs) {
                    return false;
                }
                if (*lhs++ < *rhs++) {
                    return false;
                }
            }
            return true;
		}

		// @TODO do a specific version of memory_compare that return an int to allow sorting
            //uint64_t    diff = 0;

            //// Test the first few bytes until we are 32-bit aligned.
            //while ((size & 0x3) != 0 && diff != 0)
            //{
            //    diff = (uint8_t*)lhs - (uint8_t*)rhs;
            //    size--;
            //    ((uint8_t*)lhs)++;
            //    ((uint8_t*)rhs)++;
            //}

            //// Test the next set of 32-bit integers using comparisons with
            //// aligned data.
            //while (size > sizeof(uint32_t) && diff != 0)
            //{
            //    diff = (uint32_t*)lhs - (uint32_t*)rhs;
            //    size -= sizeof(uint32_t);
            //    ((uint32_t*)lhs)++;
            //    ((uint32_t*)rhs)++;
            //}

            //// now do final bytes.
            //while (size > 0 && diff != 0)
            //{
            //    diff = (uint8_t*)lhs - (uint8_t*)rhs;
            //    size--;
            //    ((uint8_t*)lhs)++;
            //    ((uint8_t*)rhs)++;
            //}

            //return (int)*diff / abs(diff));
	}
}
