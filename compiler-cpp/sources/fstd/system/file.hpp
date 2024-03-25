#pragma once

#include "path.hpp"

#include <fstd/platform.hpp>
#include <fstd/memory/array.hpp>

#if defined(FSTD_OS_WINDOWS)
typedef void*	HANDLE;
typedef HANDLE	File_Handle;
const File_Handle Invalid_File_Handle = (File_Handle)0xffffffff'ffffffff;
#else
#	error
#endif

// @TODO
//
// Add an implementation that is garanty to be working with Windows Store like CreateFile2 instead of CreateFileW.
// As some for some applications the Windows Store will not be a target, there is no need to use restricted OS APIs.
// We may want to manage this by using a compilation flag that switch the implementation of this module.
//
// Flamaros - 26 january 2020


// @TODO check if mapping file into memory can be faster, than using traditional read by chunk method.

namespace fstd
{
	namespace stream
	{
		struct Memory_Write_Stream;
	}

	namespace system
	{
		struct File
		{
			enum class Opening_Flag
			{
				READ	= 0x01,
				WRITE	= 0x02,
				CREATE	= 0x04
			};

			constexpr static uint8_t	nb_buffers = 2;
			constexpr static size_t		buffers_size = 512;

			File_Handle					handle = Invalid_File_Handle;
			bool						is_eof = false;
		};

		bool							open_file(File& file, const Path& path, File::Opening_Flag flags);
		void							close_file(File& file);
		bool							is_file_eof(File& file);
		uint64_t						get_file_size(const File& file);
		memory::Array<uint8_t>			get_file_content(File& file);

		uint64_t						get_file_position(const File& file);
		bool							set_file_position(File& file, uint64_t position);

		bool							write_file(File& file, uint8_t* buffer, uint32_t length, uint32_t* nb_written_bytes = nullptr);
		bool							write_file(File& file, const stream::Memory_Write_Stream& stream, uint32_t* nb_written_bytes = nullptr);
	}
}
