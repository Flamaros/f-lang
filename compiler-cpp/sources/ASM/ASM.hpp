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
#include <fstd/memory/array.hpp>
#include <fstd/memory/bucket_array.hpp>
#include <fstd/memory/hash_table.hpp>
#include <fstd/stream/memory_write_stream.hpp>

// @TODO include the proper version of ASM depending on the targetted architecture
#include "ASM_x64.h"

namespace f
{
	namespace ASM
	{
		constexpr size_t NB_MAX_OPERAND_PER_INSTRUCTION = 2;
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

		struct Operand
		{
			enum class Type : uint8_t
			{
				NONE,
				REGISTER,
				IMMEDIATE,
				ADDRESS
			}				type;

			enum class Size : uint8_t	// @TODO replace that by a size in bytes???
			{
				NONE,	// when the operand isn't used
				BYTE,
				WORD,
				DOUBLE_WORD,
				QUAD_WORD
			}				size;

			// @TODO identifier (as string view?)
			// I may have to put function names as label to get addresses
			union Value
			{
				Register                _register;
				int64_t			        integer;
				uint64_t		        unsigned_integer;
				float			        real_32;
				double			        real_64;
				long double		        real_max;
			}				value;
		};

		struct Register_Desc
		{
			Operand::Size	size;
			uint8_t			id;
		};

		struct Instruction_Desc
		{
			uint8_t	opcode;
			// uint32_t	opcode;
			// uint8_t		opcode_size;
			Operand		op1;
			Operand		op2;
		};

		struct Section
		{
			fstd::language::string_view	name; // string_view of the first token parsed of this section

			fstd::stream::Memory_Write_Stream	stream_data;
			// @TODO Do I need a Bucket_Array or a Memory_Stream (maybe over the bucket array)?
			// I need something on which I can read/write, seek, get pos
			// I want chunks of memory instead of a full contiguous buffer to avoid issue with memory allocation,...
			// But I don't need to release memory, moving,... auto initialization,...

			// fstd::memory::Bucket_Array<ADDR_TO_PATCH>	addr_to_patch; // bucket_array because we need fast push_back (allocation of bucket_size) and fast iteration (over arrays)
		};

		struct Label
		{
			fstd::language::string_view	label;
			Section*					section;
			uint64_t					RVA;	// Offset in bytes relative to the beggining of the section
		};

		struct ADDR_TO_PATCH {
			fstd::language::string_view		label;			// Label to search for and obtain addr (it is a string_view of the label token)
			uint32_t						addr_of_addr;	// Where to apply the addr
		};

		struct Code
		{
			typedef fstd::memory::Hash_Table<uint16_t, fstd::language::string_view, Section*, 32> Section_Hash_Table;

			Section_Hash_Table	sections;
		};

		struct Imported_Function
		{
			fstd::language::string_view	name;
			uint32_t					name_RVA;
		};

		struct Imported_Library
		{
			typedef fstd::memory::Hash_Table<uint16_t, fstd::language::string_view, Imported_Function*, 32> Function_Hash_Table;

			fstd::language::string_view	name; // string_view of the first token parsed of this library
			Function_Hash_Table			functions;
			uint32_t					name_RVA;
		};

		struct ASM
		{
			typedef fstd::memory::Hash_Table<uint16_t, fstd::language::string_view, Imported_Library*, 32>	Imported_Library_Hash_Table;
			typedef fstd::memory::Hash_Table<uint16_t, fstd::language::string_view, Label*, 32>				Label_Hash_Table;

			Imported_Library_Hash_Table		imported_libraries;
			Label_Hash_Table				labels;
			fstd::memory::Array<Section>	sections; // @Speed an Array should be good enough, the number of sections will stay very low and search on less than 10 elements is faster on a array than a hash_table
		};

		struct ASM_Data
		{
			fstd::memory::Array<Imported_Library>	imported_libraries; // @TODO use Bucket_Array
			fstd::memory::Array<Imported_Function>	imported_functions; // @TODO use Bucket_Array
			fstd::memory::Array<Label>				labels;				// @TODO use Bucket_Array
		};

		inline bool match_section(const fstd::language::string_view& name, const Section& section) {
			return fstd::language::are_equals(name, section.name);
		}

		void compile_file(const fstd::system::Path& path, const fstd::system::Path& output_path, bool shared_library, ASM& asm_result);

		// Advanced API
		// You should create a section keep the pointer to fill it in an efficient way with function helpers when possible
		// Try to generate valid ASM code else compiler errors may happens in these functions, especially in push_instruction which
		// may fail to find the given instruction for the current targetted architecture
		Section* create_section(ASM& asm_result, fstd::language::string_view name);
		bool push_instruction(Section* section, Instruction instruction, const Operand& operand1, const Operand& operand2);
		void push_raw_data(Section* section, uint8_t* data, uint32_t size);

		Section* get_section(const ASM& asm_result, fstd::language::string_view name);

		// @TODO do the production API 
		//   - create the import module

		void debug_print(const ASM& asm_result);
	}
}
