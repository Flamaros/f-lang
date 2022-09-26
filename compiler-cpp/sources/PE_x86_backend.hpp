#pragma once

#include "IR_generator.hpp"

#include <fstd/system/path.hpp>
#include <fstd/memory/array.hpp>
#include <fstd/memory/hash_table.hpp>

namespace f
{
	struct PE_X86_Backend_Data
	{
		fstd::memory::Array<Token>		tokens;
		fstd::memory::Array<uint8_t>	instructions;
	};

	namespace PE_x86_backend
	{
		bool generate_hello_world(); // @TODO remove it

		void initialize_backend();
		void compile(IR& ir, const fstd::system::Path& output_file_path);
	}
}
