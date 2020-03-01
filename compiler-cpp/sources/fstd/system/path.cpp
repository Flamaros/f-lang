#include "path.hpp"

#include <fstd/system/allocator.hpp>

#include <fstd/core/assert.hpp>

//#include <iostream>

using namespace fstd::language;

namespace fstd
{
	namespace system
	{
		void from_native(Path& path, const std::wstring& native_path)
		{
			resize(path.string, native_path.length() + 4 + 1);	// \0 doesn't count in length()

			memory_copy(to_utf16(path.string), (void*)LR"(\\?\)", 4 * sizeof(wchar_t));
			memory_copy(&((uint16_t*)to_utf16(path.string))[4], (void*)native_path.c_str(), native_path.length() * sizeof(wchar_t));
			((uint16_t*)to_utf16(path.string))[4 + native_path.length()] = 0;
		}

		uint16_t* data(const Path& path)
		{
			return to_utf16(path.string);
		}

		void to_string(const Path& path, string& string)
		{
			core::Assert(false);
			// @TODO implement it by using utf converters
		}

		void to_string(const Path& path, utf16_string& native_string)
		{
			core::Assert(false);
			// @TODO implement it by using utf converters
		}

		utf16_string_view	to_string(const Path& path)
		{
			utf16_string_view result;

			assign(result, path.string);
			return result;
		}

		void reset_path(Path& path)
		{
			release(path.string);
		}
	}
}
