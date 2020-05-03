#pragma once

#include "path.hpp"

#include <fstd/language/string.hpp>

namespace fstd
{
	namespace system
	{
		void exit_process(int exit_code);
		bool execute_process(const system::Path& executable_path, const language::string& arguments);
	}
}
