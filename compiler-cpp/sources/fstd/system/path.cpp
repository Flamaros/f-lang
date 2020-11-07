#include "path.hpp"

#include <fstd/system/allocator.hpp>

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
