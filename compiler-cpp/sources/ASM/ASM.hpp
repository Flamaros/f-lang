#pragma once

// This file is the "public" API of the asm module.
// It aims to be used to generate the machine code and the final binary file.
// In a production stage it will provide an API to be use by compiler backend
// (IR to ASM conversion), but without using intermediate text file like almost
// all compilers are doing.
// Here we want to have something much simpler (no ASM file parsing,...).
//
// There will be ASM file loading for development and debugging purpose, but it
// shouldn't be used in production. A way to generate an ASM file from the production
// API wil be given again for development and debugging purpose.

#include <fstd/system/path.hpp>

namespace f
{
	namespace ASM
	{
		void compile_file(const fstd::system::Path& path, const fstd::system::Path& output_path, bool shared_library);
	}
}
