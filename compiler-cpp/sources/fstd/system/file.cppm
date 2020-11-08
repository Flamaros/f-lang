export module fstd.system.file;

import fstd.system.path;

#include <fstd/platform.hpp>
import fstd.memory.array;

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
	namespace system
	{
		struct File
		{
			export
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

		export bool							open_file(File& file, const Path& path, File::Opening_Flag flags);
		export void							close_file(File& file);
		export bool							is_file_eof(File& file);
		export uint64_t						get_file_size(const File& file);
		export memory::Array<uint8_t>		get_file_content(File& file);

		export memory::Array<uint8_t>		initiate_get_file_content_asynchronously(File& file);
		export bool							get_file_content_asynchronously(File& file, memory::Array<uint8_t>& buffer);	// return true if succeed
		export bool							wait_for_availabe_asynchronous_content(File& file, size_t size);	// return true if succeed

		export bool							write_file(File& file, uint8_t* buffer, uint32_t length);
	}
}

module: private;

import fstd.language.flags;
import fstd.core.unicode;
import fstd.language.string;

#if defined(FSTD_OS_WINDOWS)
#	include <Windows.h>
#endif

#undef max
#include <tracy/Tracy.hpp>
#include <tracy/common/TracySystem.hpp>

import fstd.core.assert;

// @TODO
//
// Windows Store restricted access APIS:
// - CreateFile2
// - GetFileInformationByHandle(Ex)
// - ...
//
// Flamaros - 26 january 2020

using namespace fstd;

namespace fstd
{
	namespace system
	{
		bool open_file(File& file, const Path& path, File::Opening_Flag flags)
		{
			ZoneScopedNC("fstd::system::open_file", 0x1a237e);

			core::Assert(file.handle == Invalid_File_Handle);

			file.is_eof = false;

#if defined(FSTD_OS_WINDOWS)
			DWORD	dwDesiredAccess = 0;
			DWORD	dwCreationDisposition = OPEN_EXISTING;

			if (is_flag_set(flags, File::Opening_Flag::READ)) {
				dwDesiredAccess |= GENERIC_READ;
			}
			if (is_flag_set(flags, File::Opening_Flag::WRITE)) {
				dwDesiredAccess |= GENERIC_WRITE;
			}
			if (is_flag_set(flags, File::Opening_Flag::CREATE)) {
				dwCreationDisposition = CREATE_ALWAYS;
			}

			// We have to convert the path in utf16 as Windows doesn't support UTF8.
			// We also may preprend \\?\ prefix if the path is absolut to be able to manage path longer than 260 characters.
			// @TODO

			language::UTF16LE_string	utf16_path;

			core::from_utf8_to_utf16LE(to_string(path), utf16_path, true);

			file.handle = CreateFileW(
				(LPCWSTR)language::to_utf16(utf16_path),
				dwDesiredAccess,
				0,						// dwShareMode,
				NULL,					// lpSecurityAttributes,
				dwCreationDisposition,	// dwCreationDisposition,
				FILE_ATTRIBUTE_NORMAL,	// dwFlagsAndAttributes,
				NULL					// hTemplateFile
			);
			return file.handle != INVALID_HANDLE_VALUE;
#else
#	error
#endif
			return false;
		}

		void close_file(File& file)
		{
			ZoneScopedNC("fstd::system::close_file", 0x1a237e);
#if defined(FSTD_OS_WINDOWS)
			if (file.handle != INVALID_HANDLE_VALUE) {
				CloseHandle(file.handle);
				file.handle = INVALID_HANDLE_VALUE;
			}
#else
#	error
#endif
		}

		bool is_file_eof(File& file)
		{
			return file.is_eof;
		}

		uint64_t get_file_size(const File& file)
		{
			ZoneScopedNC("fstd::system::get_file_size", 0x1a237e);

			fstd::core::Assert(file.handle != Invalid_File_Handle);
#if defined(FSTD_OS_WINDOWS)
			LARGE_INTEGER	size;

			GetFileSizeEx(file.handle, &size);

			return (uint64_t)(size.QuadPart);
#else
#	error
#endif
		}

		memory::Array<uint8_t> get_file_content(File& file)
		{
			ZoneScopedN("fstd::system::get_file_content");

#if defined(FSTD_OS_WINDOWS)
			memory::Array<uint8_t>	content;
			DWORD					read = 0;

			memory::resize_array(content, (size_t)get_file_size(file));
			if (ReadFile(
				file.handle,						// hFile
				memory::get_array_data(content),	// lpBuffer
				(DWORD)memory::get_array_size(content),	// nNumberOfBytesToRead
				&read,								// lpNumberOfBytesRead
				NULL								// lpOverlapped
			) == TRUE) {
				file.is_eof = true;
				return content;
			}
			memory::release(content);
			return content;
#else
#	error
#endif
		}

		memory::Array<uint8_t> initiate_get_file_content_asynchronously(File& file)
		{
			ZoneScopedNC("fstd::system::initiate_get_file_content_asynchronously", 0x534bae);
			// @TODO implement it correctly
			//
			// We need to initiate an asynchronous read with ReadEx and Overlapped struct
			// under Window platform.
			// We will read step by steps with a fixed size.
			//
			// The capacity of the returned buffer should be pre-allocated.
			//
			// Even this function should not lock on the first read, this will allow the user
			// to do some operation in the background.
			//
			// Flamaros - 01 february 2020
			return get_file_content(file);
		}

		bool get_file_content_asynchronously(File& file, memory::Array<uint8_t>& buffer)
		{
			ZoneScopedNC("fstd::system::get_file_content_asynchronously", 0x534bae);
			// @TODO implement it
			//
			// 
			//
			// Flamaros - 01 february 2020

#if defined(FSTD_OS_WINDOWS)
			return true;
#else
#	error
#endif
		}

		bool wait_for_availabe_asynchronous_content(File& file, size_t size)
		{
			ZoneScopedNC("fstd::system::wait_for_availabe_asynchronous_content", 0x534bae);
#if defined(FSTD_OS_WINDOWS)
			return true;
#else
#	error
#endif
		}

		bool write_file(File& file, uint8_t* buffer, uint32_t length)
		{
			ZoneScopedN("fstd::system::write_file");

			fstd::core::Assert(file.handle != Invalid_File_Handle);
#if defined(FSTD_OS_WINDOWS)
			DWORD	nb_bytes_written;

			return WriteFile(
				file.handle,		// hFile,
				buffer,				// lpBuffer,
				length,				// nNumberOfBytesToWrite,
				&nb_bytes_written,	// lpNumberOfBytesWritten,
				nullptr				// lpOverlapped
			) == TRUE;
#else
#	error
#endif
		}
	}
}
