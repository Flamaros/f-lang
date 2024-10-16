#include "file.hpp"

#include <fstd/language/flags.hpp>
#include <fstd/core/unicode.hpp>
#include <fstd/language/string.hpp>

#if defined(FSTD_OS_WINDOWS)
#	include <win32/file.h>
#	include <win32/io.h> // INVALID_HANDLE_VALUE
#endif

#include <tracy/Tracy.hpp>

#include <fstd/core/assert.hpp>

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

		uint64_t get_file_position(const File& file)
		{
			fstd::core::Assert(file.handle != Invalid_File_Handle);

#if defined(FSTD_OS_WINDOWS)
			LARGE_INTEGER	position;
			LARGE_INTEGER	zero_position;

			zero_position.QuadPart = 0;
			SetFilePointerEx(file.handle, zero_position, &position, FILE_CURRENT);

			return (uint64_t)(position.QuadPart);
#else
#	error
#endif
		}

		bool set_file_position(File& file, uint64_t position)
		{
			fstd::core::Assert(file.handle != Invalid_File_Handle);

#if defined(FSTD_OS_WINDOWS)
			LARGE_INTEGER win32_position;

			win32_position.QuadPart = position;
			return SetFilePointerEx(file.handle, win32_position, NULL, FILE_BEGIN) != 0;
#else
#	error
#endif
		}

		bool write_file(File& file, uint8_t* buffer, uint32_t length, uint32_t* nb_written_bytes)
		{
			ZoneScopedN("fstd::system::write_file");

			fstd::core::Assert(file.handle != Invalid_File_Handle);
#if defined(FSTD_OS_WINDOWS)
			return WriteFile(
				file.handle,		// hFile,
				buffer,				// lpBuffer,
				length,				// nNumberOfBytesToWrite,
				(LPDWORD)nb_written_bytes,	// lpNumberOfBytesWritten,
				nullptr				// lpOverlapped
			) == TRUE;
#else
#	error
#endif
		}
	}
}
