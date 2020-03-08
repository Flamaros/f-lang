#pragma once

#include <fstd/language/string.hpp>
#include <fstd/language/string_view.hpp>

namespace fstd
{
	namespace system
	{
		void print(const language::string_view& string);
		void print(const fstd::language::string& string);
	}
}
