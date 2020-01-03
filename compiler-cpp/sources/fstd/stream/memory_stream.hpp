#pragma once

// A stream doesn' hold the memory
// It is only made to ease the iteration

#include <fstd/memory/array.hpp>

#include <assert.h>

namespace fstd
{
	namespace stream
	{
		typedef uint32_t	Code_Point;

		struct Memory_Stream
		{
			fstd::memory::Array<uint8_t>*	buffer_ptr = nullptr;
			size_t							position = 0;
		};

		inline void initialize_memory_stream(Memory_Stream& stream, fstd::memory::Array<uint8_t>& buffer) {
			stream.buffer_ptr = &buffer;
		}

		inline bool is_eof(Memory_Stream& stream) {
			assert(stream.buffer_ptr);

			return stream.position >= stream.buffer_ptr->size;
		}

		// General
		inline void skip(Memory_Stream& stream, size_t size) {
			assert(stream.buffer_ptr);

			stream.position = stream.position + size;
		}

		inline void peek(Memory_Stream& stream) {
			skip(stream, 1);
		}

		inline uint8_t	get(Memory_Stream& stream) {
			assert(stream.buffer_ptr);

			return *memory::get_array_element(*stream.buffer_ptr, stream.position);
		}

		inline const uint8_t*	get_pointer(Memory_Stream& stream) {	// @Warning return pointer at current position
			assert(stream.buffer_ptr);

			return memory::get_array_element(*stream.buffer_ptr, stream.position);
		}

		// UTF-8
		inline bool is_uft8_bom(Memory_Stream& stream, bool skip_it) {
			assert(stream.buffer_ptr);
			assert(is_eof(stream) == false);

			bool result = *memory::get_array_element(*stream.buffer_ptr, stream.position + 0) == 0xEF
				&& *memory::get_array_element(*stream.buffer_ptr, stream.position + 1) == 0xBB
				&& *memory::get_array_element(*stream.buffer_ptr, stream.position + 2) == 0xBF;

			if (skip_it && result) {
				skip(stream, 3);
			}

			return result;
		}

		//void peek_utf8(Memory_Stream& stream) {
		//	assert(stream.buffer_ptr);
		//	assert(is_eof(stream) == false);

		//}

		//Code_Point take_utf8(Memory_Stream& stream) {
		//	assert(stream.buffer_ptr);
		//	assert(is_eof(stream) == false);

		//	

		//	if (*memory::get_array_element(*stream.buffer_ptr, stream.position) <= 0x7F) {
		//		return *memory::get_array_element(*stream.buffer_ptr, stream.position);
		//	}
		//	else if ()

		//	return 0;
		//}
	}
}
