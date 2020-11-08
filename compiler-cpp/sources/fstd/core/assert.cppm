export module fstd.core.assert;

#include "fstd/platform.hpp"

#if defined(FSTD_OS_WINDOWS)
#	include <intrin.h>
#else
#	error
#endif

namespace fstd
{
	namespace core
	{
		export void Assert(bool condition)
		{
			if (condition == false) {
				// __asm { int 3 };	// @TODO Replace by direct ASM code: debugger interrupt 0xCC opcode in x86 and 0xCCH opcode in x64
#if defined(FSTD_OS_WINDOWS)
				__debugbreak();
#else
#	error
#endif
			}
		}
	}
}
