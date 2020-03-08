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

namespace fstd
{
	namespace system
	{
		constexpr size_t Path_Header_Size = 4;

		struct Path
		{
			language::string	string;
			bool				is_absolute = false;
		};

		void					from_native(Path& path, const uint8_t* string);
		void					from_native(Path& path, const language::string& string);
		language::string_view	to_string(const Path& path);
		void					reset_path(Path& path);

		inline bool				is_absolute(const Path& path)
		{
			return path.is_absolute;
		}
	}
}
