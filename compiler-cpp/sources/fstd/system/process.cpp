#include "process.hpp"

#include <fstd/platform.hpp>

#if defined(FSTD_OS_WINDOWS)
#	include <Windows.h>
#endif

namespace fstd
{
	namespace system
	{
		void exit_process(int exit_code)
		{
#if defined(FSTD_OS_WINDOWS)
			ExitProcess(exit_code);
#endif
		}
	}
}
