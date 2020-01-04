#pragma once

#include "path.hpp"

#include <fstd/platform.hpp>
#include <fstd/memory/array.hpp>

#if defined(FSTD_OS_WINDOWS)
typedef void*	HANDLE;
typedef HANDLE	File_Handle;
#else
#	error
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

			File_Handle	handle = nullptr;
		};

		bool					open_file(File& file, const Path& path, File::Opening_Flag flags);
		void					close_file(File& file);
		uint64_t				get_file_size(const File& file);
		memory::Array<uint8_t>	get_file_content(File& file);
	}
}
