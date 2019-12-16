#pragma once

#include <fstd/platform.hpp>

#if defined(OS_WINDOWS)

namespace fstd
{
	namespace os
	{
		namespace windows
		{
			void	enable_default_console_configuration();	// This function activate default configuration for the current console or will open a new one configured correctly (to print in utf-8 with support of VT100 extensions)
			void	close_console();	// Close the console if where open by the program
		}
	}
}

#endif
