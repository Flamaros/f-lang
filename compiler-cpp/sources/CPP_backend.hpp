#pragma once

#include "IR_generator.hpp"

#include <fstd/system/path.hpp>

namespace f
{
	namespace CPP_backend
	{
		void compile(IR& irl, const fstd::system::Path& output_file_path);
	}
}
