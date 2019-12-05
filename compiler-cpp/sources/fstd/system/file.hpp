#pragma once

#include "path.hpp"

#include <fstd/platform.hpp>

#if defined(PLATFORM_WINDOWS)
typedef void*	HANDLE;
typedef HANDLE	File_Handle;
#endif

namespace fstd
{
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

			File_Handle	m_handle = nullptr;
		};

		bool	open_file(File* file, Path* const path, File::Opening_Flag flags);
		void	close_file(File* file);
	}
}
