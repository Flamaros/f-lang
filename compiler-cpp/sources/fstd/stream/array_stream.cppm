export module fstd.stream.array_stream;

// A stream doesn' hold the memory
// It is only made to ease the iteration

import fstd.language.types;

import fstd.memory.array;

import fstd.core.assert;

namespace fstd
{
	namespace stream
	{
		typedef uint32_t	Code_Point;

		export
		template<typename Array_Type>
		struct Array_Stream
		{
			fstd::memory::Array<Array_Type>*	buffer_ptr = nullptr;
			size_t								position = 0;
		};

		export
		template<typename Array_Type>
		inline void initialize_memory_stream(Array_Stream<Array_Type>& stream, fstd::memory::Array<Array_Type>& buffer) {
			stream.buffer_ptr = &buffer;
			stream.position = 0;
		}

		export
		template<typename Array_Type>
		inline bool is_eof(const Array_Stream<Array_Type>& stream) {
			fstd::core::Assert(stream.buffer_ptr);

			return stream.position >= stream.buffer_ptr->size;
		}

		export
		template<typename Array_Type>
		inline size_t get_remaining_size(const Array_Stream<Array_Type>& stream) {
			fstd::core::Assert(stream.buffer_ptr);

			if (is_eof(stream)) {
				return 0;
			}
			return stream.buffer_ptr->size - stream.position;
		}

		// General
		export
		template<typename Array_Type>
		inline void skip(Array_Stream<Array_Type>& stream, size_t size) {
			fstd::core::Assert(stream.buffer_ptr);

			stream.position = stream.position + size;
		}

		export
		template<typename Array_Type>
		inline void peek(Array_Stream<Array_Type>& stream) {
			skip(stream, 1);
		}

		export
		template<typename Array_Type>
		inline Array_Type	get(const Array_Stream<Array_Type>& stream) {
			fstd::core::Assert(stream.buffer_ptr);

			return *memory::get_array_element(*stream.buffer_ptr, stream.position);
		}

		export
		template<typename Array_Type>
		inline Array_Type*	get_pointer(const Array_Stream<Array_Type>& stream) {	// @Warning return pointer at current position
			fstd::core::Assert(stream.buffer_ptr);

			return memory::get_array_element(*stream.buffer_ptr, stream.position);
		}

		export
		template<typename Array_Type>
		inline size_t get_position(const Array_Stream<Array_Type>& stream)
		{
			return stream.position;
		}

		// UTF-8
		export
		inline bool is_uft8_bom(Array_Stream<uint8_t>& stream, bool skip_it) {
			fstd::core::Assert(stream.buffer_ptr);
			fstd::core::Assert(is_eof(stream) == false);

			bool result = *memory::get_array_element(*stream.buffer_ptr, stream.position + 0) == 0xEF
				&& *memory::get_array_element(*stream.buffer_ptr, stream.position + 1) == 0xBB
				&& *memory::get_array_element(*stream.buffer_ptr, stream.position + 2) == 0xBF;

			if (skip_it && result) {
				skip(stream, 3);
			}

			return result;
		}
	}
}
