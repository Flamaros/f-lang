#pragma once

#include "IR_generator.hpp"

#include <fstd/system/path.hpp>

namespace f
{
	struct PE_X86_Backend_Data
	{
	};

	namespace PE_x86_backend
	{
		bool generate_hello_world(); // @TODO remove it

		void initialize_backend();
		void compile(IR& ir, const fstd::system::Path& output_file_path);
	}
}
