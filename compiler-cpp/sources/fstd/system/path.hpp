#pragma once

// Unsupported character in path
//
// < (less than)
// > (greater than)
// : (colon)
// " (double quote)
// / (forward slash)
// \ (backslash)
// | (vertical bar or pipe)
// ? (question mark)
// * (asterisk)

#include <fstd/language/string.hpp>
#include <fstd/language/string_view.hpp>

#include <string>	// @TODO remove it

#include <stdint.h>	// @TODO remove it

namespace fstd
{
	namespace system
	{
		constexpr size_t Path_Header_Size = 4;

		struct Path
		{
			language::utf16_string	string;
			bool					is_absolute = false;
		};

		void						from_native(Path& path, const std::wstring& native_path);
		uint16_t*					data(const Path& path);
		language::utf16_string_view	to_string(const Path& path);
		void						to_string(const Path& path, language::string& string);
		void						to_string(const Path& path, language::utf16_string& native_string);
		void						reset_path(Path& path);

		inline bool					is_absolute(const Path& path)
		{
			return path.is_absolute;
		}
	}
}
