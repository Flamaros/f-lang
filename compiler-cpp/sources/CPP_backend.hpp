#pragma once

#include "IR_generator.hpp"

#include <fstd/system/path.hpp>

namespace f
{
	struct CPP_Backend_Data
	{
		// Incremented by one for each union declaration.
		// Mostly used to disable union member default initialization.
		size_t union_declaration_depth = 0;
	};

	namespace CPP_backend
	{
		void compile(IR& irl, const fstd::system::Path& output_file_path);
	}
}
