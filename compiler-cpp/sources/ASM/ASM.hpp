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

#include <fstd/language/types.hpp>
#include <fstd/language/string_view.hpp>
#include <fstd/system/path.hpp>
#include <fstd/memory/hash_table.hpp>

namespace f
{
	namespace ASM
	{
		//struct Imported_Function
		//{
		//	AST_Statement_Function* function;
		//	uint32_t				name_RVA;
		//};

		//struct Imported_Library
		//{
		//	typedef fstd::memory::Hash_Table<uint16_t, fstd::language::string_view, Imported_Function*, 32> Function_Hash_Table;

		//	fstd::language::string_view	name; // string_view of the first token parsed of this library
		//	Function_Hash_Table			functions;
		//	uint32_t					name_RVA;
		//};



		struct ADDR_TO_PATCH {
			fstd::language::string_view		label;			// Label to search for and obtain addr (it is a string_view of the label token)
			uint32_t						addr_of_addr;	// Where to apply the addr
		};
			
		struct Section
		{
			fstd::language::string_view	name; // string_view of the first token parsed of this section

			// fstd::memory::Bucket_Array<ADDR_TO_PATCH>	addr_to_patch; // bucket_array because we need fast push_back (allocation of bucket_size) and fast iteration (over arrays)
		};

		struct Code
		{
			typedef fstd::memory::Hash_Table<uint16_t, fstd::language::string_view, Section*, 32> Section_Hash_Table;

			Section_Hash_Table	sections;
		};

		void compile_file(const fstd::system::Path& path, const fstd::system::Path& output_path, bool shared_library);

		// @TODO do the production API 
		//   - get/create the sections
		//   - fill sections
		//   - create the import module
	}
}
