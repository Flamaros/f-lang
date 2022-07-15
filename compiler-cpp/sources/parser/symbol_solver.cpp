#include "symbol_solver.hpp"

#include "globals.hpp"
#include "parser.hpp"

#include "lexer/lexer.hpp"

#include <fstd/memory/hash_table.hpp>

#include <third-party/SpookyV2.h>

using namespace f;

static AST_Node* get_user_type(Token* identifier, Symbol_Table* symbol_table)
{
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

AST_Node* f::get_user_type(AST_User_Type_Identifier* user_type)
{
	return ::get_user_type(&user_type->identifier, user_type->symbol_table);
}

AST_Node* f::get_user_type(AST_Identifier* user_type)
{
	return ::get_user_type(&user_type->value, user_type->symbol_table);
}

AST_Node* f::resolve_type(AST_Node* user_type)
{
	if (user_type->ast_type == Node_Type::TYPE_ALIAS) {
		AST_Alias* alias = (AST_Alias*)user_type;

		return resolve_type(alias->type);
	}
	else if (user_type->ast_type == Node_Type::USER_TYPE_IDENTIFIER) {
		return resolve_type(get_user_type((AST_User_Type_Identifier*)user_type));
	}
	else if (user_type->ast_type == Node_Type::STATEMENT_IDENTIFIER) {
		return resolve_type(get_user_type((AST_Identifier*)user_type));
	}

	return user_type;
}
