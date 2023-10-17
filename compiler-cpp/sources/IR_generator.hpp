#pragma once

#include "parser/parser.hpp"

#include <fstd/language/types.hpp>
#include <fstd/language/string_view.hpp>
#include <fstd/memory/stack.hpp>
#include <fstd/memory/hash_table.hpp>

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

	struct Imported_Function
	{
		AST_Statement_Function* function;
		uint32_t				name_RVA;
	};

	struct Imported_Library
	{
		typedef fstd::memory::Hash_Table<uint16_t, fstd::language::string_view, Imported_Function*, 32> Function_Hash_Table;

		fstd::language::string_view	name; // string_view of the first token parsed of this library
		Function_Hash_Table			functions;
		uint32_t					name_RVA;
	};

	struct Literal // Things that go in rdata section like string literals
	{
		fstd::memory::Array<uint8_t>	data;
		size_t							RVA;
	};

	struct ReadOnlyData
	{
		fstd::memory::Array<Literal>	literals;
		size_t							current_RVA = 0;
	};

	struct CodeData
	{
		// RVA and addresses can't be fully computed by the IR generator, so instead the IR generator
		// simply put a relative offset from the beggining of the targeted section.
		// This mean that the backend should be able to know to which section the RVA refer.
		// For some like the entry point the RVA necesseraly point to the code section. But 
		// for instructions it may depend, a call for instance can target a RVA (or RIP offset) in the IAT or
		// an address in the code section.

		fstd::memory::Array<uint8_t>	code;
		uint32_t						entry_point_RVA = 0x00;
	};

	struct IR
	{
		typedef fstd::memory::Hash_Table<uint16_t, fstd::language::string_view, Imported_Library*, 32> Imported_Library_Hash_Table;

		Parsing_Result*					parsing_result;
		Imported_Library_Hash_Table		imported_libraries;
		ReadOnlyData					read_only_data;
		CodeData						code_data;

//		fstd::memory::Stack<Register>	registers; // Need to have one stack per scope (global scope, function scope,...)?
	};

	struct IR_Data
	{
		fstd::memory::Array<Imported_Library>	imported_libraries;
		fstd::memory::Array<Imported_Function>	imported_functions;
	};

	void generate_ir(Parsing_Result& parsing_result, IR& ir);
}
