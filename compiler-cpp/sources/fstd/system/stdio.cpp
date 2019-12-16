#include "stdio.hpp"

#include <fstd/os/windows/console.hpp>

namespace fstd
{
	namespace system
	{
		void print(const fstd::memory::String& string)
		{
#if defined(OS_WINDOWS)
			//std::wstring	string(L"\x043a\x043e\x0448\x043a\x0430 \x65e5\x672c\x56fd\n");
			//DWORD			nb_written = 0;

			//WriteConsoleW(my_stdout, string.c_str(), string.length(), &nb_written, NULL);
#else
#	error
#endif
		}
	}
}
