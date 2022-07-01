#include "symbol_solver.hpp"

#include "globals.hpp"
#include "parser.hpp"

#include "lexer/lexer.hpp"

#include <fstd/memory/hash_table.hpp>

#include <third-party/SpookyV2.h>

using namespace f;

AST_Node* f::get_user_type(AST_User_Type_Identifier* user_type)
{
	Token* identifier = &user_type->identifier;
	Symbol_Table* symbol_table = user_type->symbol_table;

	uint64_t hash = SpookyHash::Hash64((const void*)fstd::language::to_utf8(identifier->text), fstd::language::get_string_size(identifier->text), 0);
	uint16_t short_hash = hash & 0xffff;

	// @TODO do we need check shadowing here?
	// Declaration of the same type in upper scope?

	while (symbol_table)
	{
		AST_Node** type_ptr = fstd::memory::hash_table_get(symbol_table->user_types, short_hash, identifier->text);

		if (type_ptr) {
			AST_Node* type = *type_ptr;
			return type;
		}
		symbol_table = symbol_table->parent;
	}

	report_error(Compiler_Error::error, *identifier, "Unknown type.");

	// Just because the compiler request it, but we already exited the program with the error report.
	return nullptr;
}
