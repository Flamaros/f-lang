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

		inline void				copy(Path& path, const Path& source)
		{
			language::copy(path.string, 0, source.string);
			path.is_absolute = source.is_absolute;
		}

		ssize_t					extension(Path& path, language::string& extension);	// return -1 if their is no extension, else the position of character following the '.'
		ssize_t					file_name(Path& path, language::string& file_name);	// return -1 if their is no filename
		ssize_t					base_name(Path& path, language::string& base_name);	// return -1 if their is no filename
	}
}
