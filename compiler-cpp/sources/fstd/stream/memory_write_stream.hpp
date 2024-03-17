#pragma once

#include <fstd/core/assert.hpp>
#include <fstd/memory/bucket_array.hpp>

namespace fstd
{
	namespace stream
	{
		struct Memory_Write_Stream // @TODO r�cup�rer en param template la taille des buckets?
		{
			fstd::memory::Bucket_Array<uint8_t>	buffer;
			size_t								position = 0;
		};

		inline void init(Memory_Write_Stream& stream) {
			memory::init(stream.buffer);
			stream.position = 0;
		}

		inline bool is_eof(const Memory_Write_Stream& stream) {
			return stream.position >= memory::get_array_size(stream.buffer);
		}

		inline bool write(Memory_Write_Stream& stream, uint8_t* data, uint32_t size) {
			core::Assert(false);

			// Resize buffer and write (r�cup�rer la taille des buckets pour faire les memcpy sur les diff�rents buckets)
			// Il faudrait avoir une m�thode pour checker que le Bucket_Array est initialis� quand m�me (is_init?)

			return true;
		}
	}
}
