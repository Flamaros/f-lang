#pragma once

#include "parser/parser.hpp"

#include <fstd/memory/stack.hpp>

// @TODO the IR should be a valid code representation (symbols checked, types too,...)


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

	struct IR
	{
		Parsing_Result*					ast;
		fstd::memory::Stack<Register>	registers; // Need to have one stack per scope (global scope, function scope,...)?
	};

	void generate_ir(Parsing_Result& ast, IR& ir);
}
