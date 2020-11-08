export module fstd.system.path;

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

import fstd.language.string;
import fstd.language.string_view;

namespace fstd
{
	namespace system
	{
		constexpr size_t Path_Header_Size = 4;

		export
		struct Path
		{
			language::string	string;
			bool				is_absolute = false;
		};

		export void						from_native(Path& path, const uint8_t* string);
		export void						from_native(Path& path, const language::string& string);
		export language::string_view	to_string(const Path& path);
		export void						reset_path(Path& path);

		export inline bool				is_absolute(const Path& path)
		{
			return path.is_absolute;
		}
	}
}

module: private;

import fstd.system.allocator;

import fstd.core.assert;

//#include <iostream>

using namespace fstd::language;

namespace fstd
{
	namespace system
	{
		void from_native(Path& path, const uint8_t* string)
		{
			language::assign(path.string, string);
		}

		void from_native(Path& path, const language::string& string)
		{
			language::copy(path.string, 0, string);
		}

		string_view	to_string(const Path& path)
		{
			string_view result;

			assign(result, path.string);
			return result;
		}

		void reset_path(Path& path)
		{
			release(path.string);
		}
	}
}
