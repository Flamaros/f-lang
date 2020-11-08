export module fstd.system.stdio;

import fstd.language.string;
import fstd.language.string_view;

namespace fstd
{
	namespace system
	{
		export void print(const language::string_view& string);
		export void print(const fstd::language::string& string);
	}
}

module: private;

#include <fstd/platform.hpp>

import fstd.language.string_view;
import fstd.core.unicode;

import fstd.os.windows.console;

#if defined(FSTD_OS_WINDOWS)
#	include <Windows.h>
#endif

namespace fstd
{
	namespace system
	{
		void print(const language::string_view& string)
		{
#if defined(FSTD_OS_WINDOWS)
			DWORD	nb_written = 0;

			language::UTF16LE_string	utf16_string;

			core::from_utf8_to_utf16LE(string, utf16_string, false);
			WriteConsoleW(os::windows::get_std_out_handle(), language::to_utf16(utf16_string), (DWORD)language::get_string_size(utf16_string), &nb_written, NULL);
#else
#	error
#endif
		}

		void print(const language::string& string)
		{
#if defined(FSTD_OS_WINDOWS)
			DWORD	nb_written = 0;

			language::UTF16LE_string	utf16_string;

			core::from_utf8_to_utf16LE(string, utf16_string, false);
			WriteConsoleW(os::windows::get_std_out_handle(), language::to_utf16(utf16_string), (DWORD)language::get_string_size(utf16_string), &nb_written, NULL);
#else
#	error
#endif
		}
	}
}
