#include "allocator.hpp"

#include <fstd/platform.hpp>

#if defined(FSTD_OS_WINDOWS)
#	include <Windows.h>
#endif

//#include <cmath>
#include <fstd/core/assert.hpp>

// Need we use VirtualAlloc instead of HeapAlloc, to support allocation of bigger buffers?
// A size of 16MB seems good.

// We use some bits operations to reduce the memory footprint and improve performances.
// Size of the Memory_Header struct is a critical point.
//
// Flamaros - 14 january 2020
//
// https://stackoverflow.com/questions/47981/how-do-you-set-clear-and-toggle-a-single-bit


//#define POOL_32

namespace fstd
{
	namespace system
	{
		void* os_allocate(size_t size)
		{
#if defined(FSTD_OS_WINDOWS)
			return HeapAlloc(GetProcessHeap(), 0, size);
#else
#	error
#endif
		}

		void* os_reallocate(void* address, size_t size)
		{
#if defined(FSTD_OS_WINDOWS)
			return HeapReAlloc(GetProcessHeap(), 0, address, size);
#else
#	error
#endif
		}

		void os_free(void* address)
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

#if defined(FSTD_X86_64)
		typedef uint64_t	Flag_Set;
#else
		typedef uint32_t	Flag_Set;
#endif

		constexpr uint32_t	check_32_count = 1'024;
		constexpr uint32_t	flag_set_count = check_32_count / (sizeof(Flag_Set) * 8);
//		constexpr uint32_t	flag_set_count_bits = ~flag_set_count & (flag_set_count + 1);	// @TODO we need an intrinsic here like ctz https://en.wikipedia.org/wiki/Find_first_set, but to do it a compile time we need a better language
		constexpr uint32_t	flag_set_first_bit_index_to_set = (sizeof(Flag_Set) * 8) - 1;

		enum class Chunk_Size : uint16_t
		{
			not_used,
			size_32,
		};

		struct Memory_Header
		{
			// Bitfield of 16 bits
			uint16_t	set_index	: 6;	// For 1024 chunks we need only 16 sets => 4 bits, but we can resonably give little bits if we want to handle more chunks
			uint16_t	bit_index	: 6;	// can't exceed 63, need only 6 bits, other bits are used for the set_index
			Chunk_Size	size		: 4;	// We take only 4 bits for 16 values
		};

		constexpr uint32_t	Memory_Header_size = sizeof(Memory_Header);
		constexpr uint16_t	flag_set_size_in_bits = sizeof(Flag_Set) * 8;

#if defined(POOL_32)
		void*				chunks_32 = nullptr;
		Flag_Set*			chunks_32_flags = nullptr;
#endif

		inline void free_32(Memory_Header* index)
		{
#if defined(POOL_32)
			chunks_32_flags[index->set_index] &= ~(1ULL << index->bit_index);
#endif
		}
		
		inline void* allocate_32(size_t size)
		{
#if defined(POOL_32)
			if (size > 32) {
				return nullptr;
			}

			for (uint16_t i = 0; i < flag_set_count; i++) {
#if defined(FSTD_OS_WINDOWS)
				DWORD	bit_index;

#if defined(FSTD_X86_64)
				if (_BitScanForward64(&bit_index, chunks_32_flags[i])) {	// @TODO We need to implement this intrinsic in f-lang
#else
				if (_BitScanForward(&bit_index, chunks_32_flags[i])) {	// @TODO We need to implement this intrinsic in f-lang
#endif
					bit_index -= 1;
				}
				else {
					bit_index = flag_set_first_bit_index_to_set;
				}
				chunks_32_flags[i] |= 1ULL << bit_index;

				void* ptr = (void*)((size_t)chunks_32 + (size_t)((uint32_t)i * flag_set_size_in_bits + bit_index) * 32);
				((Memory_Header*)ptr)->set_index = i;
				((Memory_Header*)ptr)->bit_index = bit_index;
				((Memory_Header*)ptr)->size = Chunk_Size::size_32;
				return ptr;
#else
#	error
#endif
			}
#endif
			return nullptr;
		}

		// was_32_allocation is filled only in case of failure
		inline void* reallocate_32(Memory_Header* header, size_t size, bool* was_32_allocation)
		{
#if defined(POOL_32)
			if (size <= 32) {
				// If header->size > 32, we can return the address as the user can later do an other reallocate with a bigger size
				// If the size changed but is still under 32 bytes, then the user_address is still valid
				//
				// Flamaros - 11 january 2020
				return header;	// header address is the same as the resulting one of the previous allocation
			}
			else {
				if (header->size == Chunk_Size::size_32) {
					*was_32_allocation = true;
				}
				else {
					*was_32_allocation = false;
				}
				return nullptr;
			}
#else
			* was_32_allocation = false;
			return nullptr;
#endif
		}

		// =====================================================================

		void allocator_initialize()
		{
			fstd::core::Assert(sizeof(Memory_Header) == 2);

#if defined(POOL_32)
			if (chunks_32 == nullptr) {
				chunks_32 = os_allocate(check_32_count * 32);
				chunks_32_flags = (Flag_Set*)os_allocate(flag_set_count * sizeof(Flag_Set));
				zero_memory(chunks_32_flags, flag_set_count * sizeof(Flag_Set));
			}
#endif
		}

		void* allocate(size_t size)
		{
			size_t	real_size = size + Memory_Header_size;
			void*	ptr;

			if ((ptr = allocate_32(real_size)) == nullptr) {
				ptr = os_allocate(real_size);
				((Memory_Header*)ptr)->size = Chunk_Size::not_used;
			}

			return (void*)((size_t)ptr + Memory_Header_size);
		}

		void* reallocate(void* address, size_t size)
		{
			size_t	real_size = size + sizeof(Memory_Header);
			void*	ptr;

			if (address) {
				Memory_Header*	header = (Memory_Header*)((size_t)address - Memory_Header_size);
				bool			was_32_allocation;

				if ((ptr = reallocate_32(header, real_size, &was_32_allocation)) == nullptr) {
					if (was_32_allocation) {
						ptr = os_allocate(real_size);

						memory_copy(ptr, header, 32 + Memory_Header_size);
						free_32(header);
						((Memory_Header*)ptr)->size = Chunk_Size::not_used;
					}
					else {
						ptr = os_reallocate((void*)header, real_size);
					}
				}
			}
			else {
				if ((ptr = allocate_32(real_size)) == nullptr) {
					ptr = os_allocate(real_size);
					((Memory_Header*)ptr)->size = Chunk_Size::not_used;
				}
			}
			return (void*)((size_t)ptr + Memory_Header_size);
		}

		void free(void* address)
		{
			if (address == 0x00)
				return;

			Memory_Header* header = (Memory_Header*)((size_t)address - Memory_Header_size);

			if (header->size == Chunk_Size::size_32) {
				free_32(header);
			}
			else {
				os_free((void*)((size_t)address - Memory_Header_size));
			}
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
