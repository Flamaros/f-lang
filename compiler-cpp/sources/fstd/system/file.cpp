#include "file.hpp"

#include <fstd/language/flags.hpp>

#if defined(PLATFORM_WINDOWS)
#	include <Windows.h>
#endif

namespace fstd
{
	namespace system
	{
		bool open_file(File& file, const Path& path, File::Opening_Flag flags)
		{
			close_file(file);

#if defined(PLATFORM_WINDOWS)
			DWORD	dwDesiredAccess = 0;
			DWORD	dwCreationDisposition = OPEN_EXISTING;

			if (utilities::is_flag_set(flags, File::Opening_Flag::READ)) {
				dwDesiredAccess |= GENERIC_READ;
			}
			if (utilities::is_flag_set(flags, File::Opening_Flag::WRITE)) {
				dwDesiredAccess |= GENERIC_WRITE;
			}
			if (utilities::is_flag_set(flags, File::Opening_Flag::CREATE)) {
				dwDesiredAccess = CREATE_ALWAYS;
			}

			file.handle = CreateFileW(
				to_native(path),
				dwDesiredAccess,
				0,						// dwShareMode,
				NULL,					// lpSecurityAttributes,
				OPEN_EXISTING,			// dwCreationDisposition,
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
#if defined(PLATFORM_WINDOWS)
			if (file.handle != nullptr) {
				CloseHandle(file.handle);
				file.handle = nullptr;
			}
#else
#	error
#endif
		}

		uint64_t get_file_size(const File& file)
		{
#if defined(PLATFORM_WINDOWS)
			LARGE_INTEGER	size;

			GetFileSizeEx(file.handle, &size);

			return (uint64_t)(size.QuadPart);
#else
#	error
#endif
		}

		memory::Array<uint8_t> get_file_content(File& file)
		{
#if defined(PLATFORM_WINDOWS)
			memory::Array<uint8_t>	content;
			DWORD					read = 0;

			memory::resize_array(content, (size_t)get_file_size(file));
			if (ReadFile(
				file.handle,						// hFile
				memory::get_array_data(content),	// lpBuffer
				memory::get_array_size(content),	// nNumberOfBytesToRead
				&read,								// lpNumberOfBytesRead
				NULL								// lpOverlapped
			) == TRUE) {
				return content;
			}
			memory::reset_array(content);
			return content;
#else
#	error
#endif
		}
	}
}
