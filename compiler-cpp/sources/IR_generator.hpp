#pragma once

#include "parser/parser.hpp"

#include <fstd/memory/stack.hpp>

namespace f
{
	struct Register
	{
		enum class Type // Basic types (CPU types) only are supported here (no structs,...)
		{
			BYTE		= 0x01,
			WORD		= 0x02,
			DWORD		= 0x04,
			QWORD		= 0x08,
			FLOAT		= 0x10,
			DOUBLE		= 0x20,
			UNSIGNED	= 0x40,
			POINTER		= 0x80
		};

		Type		type;
		uint32_t	id;
	};

	struct Imported_Library
	{
		typedef fstd::memory::Hash_Table<uint16_t, fstd::language::string_view, AST_Node*, 32> Function_Hash_Table;

		fstd::language::string_view	name; // string_view of the first token parsed of this library
		Function_Hash_Table			functions;
	};

	struct IR
	{
		typedef fstd::memory::Hash_Table<uint16_t, fstd::language::string_view, Imported_Library*, 32> Imported_Library_Hash_Table;

		Parsing_Result*				parsing_result;
		Imported_Library_Hash_Table	imported_libraries;

//		fstd::memory::Stack<Register>	registers; // Need to have one stack per scope (global scope, function scope,...)?
	};

	void generate_ir(Parsing_Result& parsing_result, IR& ir);
}
