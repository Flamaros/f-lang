#include "file.hpp"

#include <utilities/flags.hpp>

#if defined(PLATFORM_WINDOWS)
#	include <Windows.h>
#endif

namespace fstd
{
	namespace system
	{
		bool	open_file(File* file, Path* const path, File::Opening_Flag flags)
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

			file->m_handle = CreateFileW(
				to_native(path),
				dwDesiredAccess,
				0,						// dwShareMode,
				NULL,					// lpSecurityAttributes,
				OPEN_EXISTING,			// dwCreationDisposition,
				FILE_ATTRIBUTE_NORMAL,	// dwFlagsAndAttributes,
				NULL					// hTemplateFile
			);
			return file->m_handle != INVALID_HANDLE_VALUE;
#else
#	error
#endif
			return false;
		}

		void	close_file(File* file)
		{
#if defined(PLATFORM_WINDOWS)
			if (file->m_handle != nullptr) {
				CloseHandle(file->m_handle);
				file->m_handle = nullptr;
			}
#else
#	error
#endif
		}
	}
}
