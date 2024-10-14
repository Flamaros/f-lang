#pragma once

#include <fstd/core/assert.hpp>
#include <fstd/memory/bucket_array.hpp>

namespace fstd
{
	namespace stream
	{
		// @TODO
		// Need I support cursor moves?
		// Need I support reads? With a different cursor than write?

		struct Memory_Write_Stream // @TODO récupérer en param template la taille des buckets?
		{
			memory::Bucket_Array<uint8_t>	buffer;
			ssize_t							position = 0;
		};

		inline void init(Memory_Write_Stream& stream) {
			memory::init(stream.buffer);
			stream.position = 0;
		}

		inline bool is_eof(const Memory_Write_Stream& stream) {
			return stream.position >= memory::get_array_size(stream.buffer);
		}

		inline ssize_t get_size(const Memory_Write_Stream& stream) {
			return memory::get_array_size(stream.buffer);
		}

		inline bool set_position(Memory_Write_Stream& stream, ssize_t position) {
			if (position >= 0 && position < get_size(stream)) {
				stream.position = position;
				return true;
			}
			return false;
		}

		inline ssize_t get_position(const Memory_Write_Stream& stream) {
			return stream.position;
		}

		inline void reset(Memory_Write_Stream& stream) {
			stream.position = 0;
			memory::reset_array(stream.buffer);
		}

		inline bool write(Memory_Write_Stream& stream, uint8_t* data, uint32_t size) {
			memory::push_back(stream.buffer, data, size);
			stream.position += size;

			return true;
		}

		inline const memory::Bucket_Array<uint8_t>& get_buffer(const Memory_Write_Stream& stream) {
			return stream.buffer;
		}
	}
}
