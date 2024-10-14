#include "path.hpp"

#include <fstd/system/memory.hpp>

#include <fstd/core/assert.hpp>
#include <fstd/math/math.hpp>

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

		ssize_t base_name(Path& path, language::string& base_name)
        {
			ssize_t	file_name_pos1 = language::last_of(path.string, '\\');
			ssize_t	file_name_pos2 = language::last_of(path.string, '/');
			ssize_t	file_name_pos = math::max(file_name_pos1, file_name_pos2) + 1; // + 1 to skip the folder seperator character
			ssize_t	extension_pos = language::first_of(path.string, '.', file_name_pos);

			language::copy(base_name, 0, path.string, file_name_pos, extension_pos - file_name_pos);

            return file_name_pos;
        }
	}
}
