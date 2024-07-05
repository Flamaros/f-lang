#include "file.hpp"

#include <fstd/language/flags.hpp>
#include <fstd/language/string.hpp>
#include <fstd/core/unicode.hpp>
#include <fstd/core/assert.hpp>
#include <fstd/stream/memory_write_stream.hpp>
#include <fstd/memory/bucket_array.hpp>
#include <fstd/system/allocator.hpp>

#if defined(FSTD_OS_WINDOWS)
#	include <win32/file.h>
#	include <win32/io.h> // INVALID_HANDLE_VALUE
#endif

#include <tracy/Tracy.hpp>

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

		bool write_file(File& file, const stream::Memory_Write_Stream& stream, uint32_t* nb_written_bytes)
		{
			const auto& bucket_array = stream::get_buffer(stream);
			uint32_t	array_size = (uint32_t)memory::get_array_size(bucket_array);
			uint32_t	remaining_size = array_size;
			uint32_t	temp_nb_written_bytes = 0;
			uint32_t	bucket_size = (uint32_t)memory::get_bucket_size(bucket_array); // Store the contexpr locally to avoid many useless instances of this function

			if (nb_written_bytes) {
				*nb_written_bytes = 0;
			}
			while (remaining_size)
			{
				size_t		position = array_size - remaining_size;
				size_t		bucket_index = position / bucket_size;
				uint32_t	length = remaining_size > bucket_size ? bucket_size : remaining_size;
				if (write_file(file, memory::get_bucket(bucket_array, bucket_index), length, &temp_nb_written_bytes) == false) {
					return false;
				}
				remaining_size -= temp_nb_written_bytes;
				if (nb_written_bytes) {
					*nb_written_bytes += temp_nb_written_bytes;
				}
			}

			return true;
		}

		bool write_zeros(File file, uint32_t count)
		{
			bool				result = true;
			constexpr uint32_t	buffer_size = 512;
			static bool			initialized = false;
			static char			zeros_buffer[buffer_size];
			uint32_t			nb_iterations = count / buffer_size;
			uint32_t			modulo = count % buffer_size;
			uint32_t			bytes_written;

			if (initialized == false) {
				fstd::system::zero_memory(zeros_buffer, buffer_size);
				initialized = true;
			}

			for (uint32_t i = 0; result && i < nb_iterations; i++) {
				result &= write_file(file, (uint8_t*)zeros_buffer, buffer_size, &bytes_written);
			}
			if (result) {
				result &= write_file(file, (uint8_t*)zeros_buffer, modulo, &bytes_written);
			}

			return result;
		}
	}
}
