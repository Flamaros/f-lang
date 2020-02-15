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

#include <string>	// @TODO remove it

#include <stdint.h>	// @TODO remove it

namespace fstd
{
	namespace system
	{
		struct Path
		{
			void*	buffer = nullptr;
			size_t	length = 0;
			bool	is_absolute = false;
		};

		void		from_native(Path& path, const std::wstring& native_path);
		wchar_t*	to_native(const Path& path);
		void		reset_path(Path& path);
	}
}
