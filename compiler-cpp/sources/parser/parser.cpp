#include "parser.hpp"

#include "globals.hpp"

#include <fstd/core/logger.hpp>
#include <fstd/core/string_builder.hpp>

#include <fstd/container/array.hpp>
#include <fstd/container/hash_table.hpp>

#include <fstd/stream/array_read_stream.hpp>

#include <fstd/system/file.hpp>
#include <fstd/system/stdio.hpp>

#include <fstd/language/defer.hpp>
#include <fstd/language/flags.hpp>
#include <fstd/language/string.hpp>
#include <fstd/language/string_view.hpp>

#include <third-party/SpookyV2.h>

#include <tracy/Tracy.hpp>

#include <magic_enum/magic_enum.hpp> // @TODO remove it

// Typedef complexity
// https://en.cppreference.com/w/cpp/language/typedef

using namespace fstd;
using namespace fstd::core;

using namespace f;

enum class State
{
	GLOBAL_SCOPE,
	COMMENT_LINE,
	COMMENT_BLOCK,
	IMPORT_DIRECTIVE,

	eof
};

template<typename Node_Type>
inline Node_Type* allocate_AST_node(AST_Node** emplace_node)
{
	ZoneScopedN("allocate_AST_node");

	// Ensure that no reallocation could happen during the resize
	bool overflow_preallocated_buffer = container::get_array_size(globals.parser_data.ast_nodes) >= container::get_array_reserved(globals.parser_data.ast_nodes) * (ssize_t)sizeof(AST_Statement_Variable);

	core::Assert(overflow_preallocated_buffer == false);
	if (overflow_preallocated_buffer) {
		report_error(Compiler_Error::internal_error, "The compiler did not allocate enough memory to store AST_Node!");
	}

	// @TODO use a version that didn't check the array size, because we just did it and we don't want to trigger unplanned allocations
	container::resize_array(globals.parser_data.ast_nodes, container::get_array_size(globals.parser_data.ast_nodes) + sizeof(Node_Type));

	Node_Type* new_node = (Node_Type*)(container::get_array_last_element(globals.parser_data.ast_nodes) - sizeof(Node_Type));
	if (emplace_node) {
		*emplace_node = (AST_Node*)new_node;
	}
	return new_node;
}

inline Symbol_Table* allocate_symbol_table()
{
	ZoneScopedN("allocate_symbol_table");

	// Ensure that no reallocation could happen during the resize
	bool overflow_preallocated_buffer = container::get_array_size(globals.parser_data.symbol_tables) >= container::get_array_reserved(globals.parser_data.symbol_tables) * (ssize_t)sizeof(Symbol_Table);

	core::Assert(overflow_preallocated_buffer == false);
	if (overflow_preallocated_buffer) {
		report_error(Compiler_Error::internal_error, "The compiler did not allocate enough memory to store Symbol_Table!");
	}

	// @TODO use a version that didn't check the array size, because we just did it and we don't want to trigger unplanned allocations
	container::resize_array(globals.parser_data.symbol_tables, container::get_array_size(globals.parser_data.symbol_tables) + sizeof(Symbol_Table));

	Symbol_Table* new_node = (Symbol_Table*)(container::get_array_last_element(globals.parser_data.symbol_tables) - sizeof(Symbol_Table));
	return new_node;
}

// =============================================================================

static void parse_array(stream::Array_Read_Stream<Token<Keyword>>& stream, AST_Statement_Type_Array** array_node_);
static void parse_type(stream::Array_Read_Stream<Token<Keyword>>& stream, AST_Node** type_node);
static void parse_variable(stream::Array_Read_Stream<Token<Keyword>>& stream, Token<Keyword>& identifier, AST_Statement_Variable** variable_, bool is_function_parameter = false);
static inline void parse_alias(stream::Array_Read_Stream<Token<Keyword>>& stream, Token<Keyword>& identifier, AST_Node** previous_sibling_addr);
static void parse_function(stream::Array_Read_Stream<Token<Keyword>>& stream, Token<Keyword>& identifier, AST_Node** previous_sibling_addr);
static void parse_function_call(stream::Array_Read_Stream<Token<Keyword>>& stream, Token<Keyword>& identifier, AST_Function_Call** emplace_node);
static void parse_struct(stream::Array_Read_Stream<Token<Keyword>>& stream, Token<Keyword>* identifier, AST_Node** previous_sibling_addr); /// @param identifier If null the union is anonymous
static void parse_enum(stream::Array_Read_Stream<Token<Keyword>>& stream, Token<Keyword>& identifier, AST_Node** previous_sibling_addr);
static void parse_union(stream::Array_Read_Stream<Token<Keyword>>& stream, Token<Keyword>* identifier, AST_Node** previous_sibling_addr); /// @param identifier If null the union is anonymous
static void parse_binary_operator(stream::Array_Read_Stream<Token<Keyword>>& stream, AST_Node** emplace_node, Node_Type node_type, AST_Node** previous_child, Punctuation delimiter_1, Punctuation delimiter_2);
static void fix_operations_order(AST_Binary_Operator* binary_operator_node);
static void parse_unary_operator(stream::Array_Read_Stream<Token<Keyword>>& stream, AST_Node** emplace_node, Node_Type node_type, AST_Node** previous_child, Punctuation delimiter_1, Punctuation delimiter_2);
static bool parse_expression(stream::Array_Read_Stream<Token<Keyword>>& stream, AST_Node** emplace_node, Punctuation delimiter_1, Punctuation delimiter_2 = Punctuation::UNKNOWN); // Return true if the expression is scoped by parenthesis
static void parse_scope(stream::Array_Read_Stream<Token<Keyword>>& stream, AST_Statement_Scope** scope_node_, bool is_root_node = false);
static void parse_struct_scope(stream::Array_Read_Stream<Token<Keyword>>& stream, AST_Statement_Struct_Type* scope_node_, bool is_root_node = false);
static void parse_union_scope(stream::Array_Read_Stream<Token<Keyword>>& stream, AST_Statement_Union_Type* scope_node_, bool is_root_node = false);

static void initialize_symbol_table(Symbol_Table* symbol_table, Symbol_Table* parent, Symbol_Table* sibling, Scope_Type type, Token<Keyword>* name);

// =============================================================================

// Allocate a new symbol_table of requested type, and make it current
// Automatically put the scope as first_child or as sibling of first_child depending on the situration of current_node
inline void push_new_symbol_table(Scope_Type type, Token<Keyword>* name)
{
	ZoneScopedN("push_new_symbol_table");

	Symbol_Table* parent = globals.parser_data.current_symbol_table;

	// Actually as symbols don't leak from there scope and the lookup will be only based on parents, we don't have to keep children in declaration order.
	// So the new symbol table will be put at first child and current first_child will be put as sibling of new_symbol_table.

	Symbol_Table* new_symbol_table = allocate_symbol_table();
	if (parent) {
		initialize_symbol_table(new_symbol_table, parent, parent->first_child, type, name);
		parent->first_child = new_symbol_table;
	}
	else {
		initialize_symbol_table(new_symbol_table, parent, nullptr, type, name);
	}
	globals.parser_data.current_symbol_table = new_symbol_table;
}

// Update the current_symbol_table to the parent
inline void pop_symbol_table()
{
	globals.parser_data.current_symbol_table = globals.parser_data.current_symbol_table->parent;
}

// =============================================================================

void parse_array(stream::Array_Read_Stream<Token<Keyword>>& stream, AST_Statement_Type_Array** array_node_)
{
	ZoneScopedN("parse_array");

	Token<Keyword>	current_token;

	current_token = stream::get(stream);
	core::Assert(current_token.type == Token_Type::SYNTAXE_OPERATOR && current_token.value.punctuation == Punctuation::OPEN_BRACKET);

	AST_Statement_Type_Array*	array_node = allocate_AST_node<AST_Statement_Type_Array>(nullptr);
	array_node->ast_type = Node_Type::STATEMENT_TYPE_ARRAY;
	array_node->sibling = nullptr;
	array_node->array_size = nullptr;

	*array_node_ = array_node;

	stream::peek(stream); // [

	parse_expression(stream, (AST_Node**)&array_node->array_size, Punctuation::CLOSE_BRACKET);

	stream::peek(stream); // ]
}

void parse_type(stream::Array_Read_Stream<Token<Keyword>>& stream, AST_Node** type_node)
{
	ZoneScopedN("parse_type");

	Token<Keyword>		current_token;
	AST_Node**	previous_sibling_addr = nullptr;

	// A type sequence will necessary ends with the type name (basic type keyword or identifier), all modifiers comes before.
	//
	// Flamaros - 01 may 2020

	while (true)
	{
		current_token = stream::get(stream);

		if (current_token.type == Token_Type::SYNTAXE_OPERATOR) {
			if (current_token.value.punctuation == Punctuation::SECTION) {
				// @TODO create a parse_pointer
				AST_Statement_Type_Pointer*	pointer_node = allocate_AST_node<AST_Statement_Type_Pointer>(previous_sibling_addr);
				previous_sibling_addr = (AST_Node**)&pointer_node->sibling;

				if (*type_node == nullptr) {
					*type_node = (AST_Node*)pointer_node;
				}

				pointer_node->ast_type = Node_Type::STATEMENT_TYPE_POINTER;
				pointer_node->sibling = nullptr;

				stream::peek(stream); // §
			}
			else if (current_token.value.punctuation == Punctuation::OPEN_BRACKET) {
				AST_Statement_Type_Array* array_node;

				parse_array(stream, &array_node);
				if (previous_sibling_addr) {
					*previous_sibling_addr = (AST_Node*)array_node;
				}
				previous_sibling_addr = &array_node->sibling;

				if (*type_node == nullptr) {
					*type_node = (AST_Node*)array_node;
				}
			}
			else {
				report_error(Compiler_Error::error, current_token, "Expecting a type qualifier (a type modifier operator, a basic type keyword or a user type identifier).");
			}
		}
		else if (current_token.type == Token_Type::KEYWORD) {
			if (f::is_a_basic_type(current_token.value.keyword)) {
				AST_Statement_Basic_Type*	basic_type_node = allocate_AST_node<AST_Statement_Basic_Type>(previous_sibling_addr);

				if (*type_node == nullptr) {
					*type_node = (AST_Node*)basic_type_node;
				}

				basic_type_node->ast_type = Node_Type::STATEMENT_BASIC_TYPE;
				basic_type_node->sibling = nullptr;
				basic_type_node->keyword = current_token.value.keyword;
				basic_type_node->token = current_token;

				stream::peek(stream); // basic_type keyword
				break;
			}
			else if (current_token.value.keyword == Keyword::STRUCT) {
				stream::peek(stream); // struct
				parse_struct(stream, nullptr, type_node);
				break;
			}
			else if (current_token.value.keyword == Keyword::UNION) {
				stream::peek(stream); // struct
				parse_union(stream, nullptr, type_node);
				break;
			}
			else {
				report_error(Compiler_Error::error, current_token, "Expecting a type qualifier (a type modifier operator, a basic type keyword or a user type identifier).");
			}
		}
		else if (current_token.type == Token_Type::IDENTIFIER) {
			AST_User_Type_Identifier*	user_type_node = allocate_AST_node<AST_User_Type_Identifier>(previous_sibling_addr);

			if (*type_node == nullptr) {
				*type_node = (AST_Node*)user_type_node;
			}

			user_type_node->ast_type = Node_Type::USER_TYPE_IDENTIFIER;
			user_type_node->sibling = nullptr;
			user_type_node->identifier = current_token;
			user_type_node->symbol_table = globals.parser_data.current_symbol_table;

			stream::peek(stream); // identifier
			break;
		}
		else {
			report_error(Compiler_Error::error, current_token, "Expecting a type qualifier (a type modifier operator, a basic type keyword or a user type identifier).");
		}
	}
}

void parse_variable(stream::Array_Read_Stream<Token<Keyword>>& stream, Token<Keyword>& identifier, AST_Statement_Variable** variable_, bool is_function_parameter /* = false */)
{
	ZoneScopedN("parse_variable");

	Token<Keyword>					current_token;
	AST_Statement_Variable* variable;

	current_token = stream::get(stream);
	fstd::core::Assert(current_token.type == Token_Type::SYNTAXE_OPERATOR && current_token.value.punctuation ==  Punctuation::COLON);

	variable = allocate_AST_node<AST_Statement_Variable>(nullptr);
	variable->ast_type = Node_Type::STATEMENT_VARIABLE;
	variable->sibling = nullptr;
	variable->name = identifier;
	variable->type = nullptr;
	variable->is_function_parameter = is_function_parameter;
	variable->is_optional = false;
	variable->expression = nullptr;

	*variable_ = variable;

	stream::peek(stream); // :

	parse_type(stream, &variable->type);
	current_token = stream::get(stream);

	if (current_token.type == Token_Type::SYNTAXE_OPERATOR
		&& current_token.value.punctuation == Punctuation::EQUALS) {
		if (is_function_parameter) {
			variable->is_optional = true;
		}

		stream::peek(stream); // =

		if (is_function_parameter) {
			parse_expression(stream, (AST_Node**)&variable->expression, Punctuation::COMMA, Punctuation::CLOSE_PARENTHESIS);
		}
		else {
			parse_expression(stream, (AST_Node**)&variable->expression, Punctuation::SEMICOLON);
		}
	}

	if (is_function_parameter == false &&	// @Warning the parse_function method have to be able to read arguments delimiters ',' or ')' characters
		// @Warning struct and union can be anonymous, in this case the type declaration is made directly in the variable declaration
		// and the variable declaration ends with the struct or union one (so there is no ; expected here)
		variable->type->ast_type != Node_Type::STATEMENT_TYPE_STRUCT &&
		variable->type->ast_type != Node_Type::STATEMENT_TYPE_UNION) {
		stream::peek(stream); // ;
	}

	// symbol table
	if (!variable->is_function_parameter)
	{
		uint64_t hash = SpookyHash::Hash64((const void*)fstd::language::to_utf8(variable->name.text), fstd::language::get_string_size(variable->name.text), 0);
		uint16_t short_hash = hash & 0xffff;
		AST_Node* value = (AST_Node*)variable;

		fstd::container::hash_table_insert(globals.parser_data.current_symbol_table->variables, short_hash, variable->name.text, value);
	}
}

void parse_alias(stream::Array_Read_Stream<Token<Keyword>>& stream, Token<Keyword>& identifier, AST_Node** previous_sibling_addr)
{
	ZoneScopedN("parse_alias");

	AST_Alias*	alias_node = allocate_AST_node<AST_Alias>(previous_sibling_addr);

	alias_node->ast_type = Node_Type::TYPE_ALIAS;
	alias_node->sibling = nullptr;
	alias_node->name = identifier;

	parse_expression(stream, &alias_node->type, Punctuation::SEMICOLON);
	stream::peek(stream); // ;

	// symbol table
	{
		uint64_t hash = SpookyHash::Hash64((const void*)fstd::language::to_utf8(alias_node->name.text), fstd::language::get_string_size(alias_node->name.text), 0);
		uint16_t short_hash = hash & 0xffff;
		AST_Node* value = (AST_Node*)alias_node;

		fstd::container::hash_table_insert(globals.parser_data.current_symbol_table->user_types, short_hash, alias_node->name.text, value);
	}
}

void parse_function(stream::Array_Read_Stream<Token<Keyword>>& stream, Token<Keyword>& identifier, AST_Node** previous_sibling_addr)
{
	ZoneScopedN("parse_function");

	Token<Keyword>					current_token;
	AST_Statement_Function*	function_node = allocate_AST_node<AST_Statement_Function>(previous_sibling_addr);

	function_node->ast_type = Node_Type::STATEMENT_FUNCTION;
	function_node->sibling = nullptr;
	function_node->name = identifier;
	function_node->nb_arguments = 0;
	function_node->arguments = nullptr;
	function_node->return_type = nullptr;
	function_node->scope = nullptr;

	current_token = stream::get(stream);

	AST_Statement_Variable**	current_argument = &function_node->arguments;
	while (!(current_token.type == Token_Type::SYNTAXE_OPERATOR
		&& current_token.value.punctuation == Punctuation::CLOSE_PARENTHESIS))
	{
		if (current_token.type == Token_Type::IDENTIFIER)
		{
			Token<Keyword>	identifier = current_token;

			stream::peek(stream); // identifier
			current_token = stream::get(stream);

			if (!(current_token.type == Token_Type::SYNTAXE_OPERATOR
				&& current_token.value.punctuation == Punctuation::COLON)) {
				report_error(Compiler_Error::error, current_token, "Expecting ':' between the type and the name of the paramater of function.");
			}

			parse_variable(stream, identifier, current_argument, true);
			current_token = stream::get(stream);

			function_node->nb_arguments++;
			current_argument = (AST_Statement_Variable**)&((*current_argument)->sibling);
			if (current_token.type == Token_Type::SYNTAXE_OPERATOR &&
				current_token.value.punctuation == Punctuation::COMMA)
			{
				stream::peek(stream); // ,
				current_token = stream::get(stream);
			}
		}
		else {
			report_error(Compiler_Error::error, current_token, "Expecting an identifier for the paramater name of the function");
		}
	}
	stream::peek(stream); // )

	current_token = stream::get(stream);
	if (current_token.type == Token_Type::SYNTAXE_OPERATOR
		&& current_token.value.punctuation == Punctuation::ARROW) {
		stream::peek(stream); // ->
		parse_type(stream, &function_node->return_type); // @TODO get the return type and put it in the tree
		current_token = stream::get(stream);
	}

	// @TODO I am not sure that using a lambda is ideal (I a not sure that I will add a similar feature in f-lang)
	auto insert_to_symbol_table = [&]() {
		uint64_t hash = SpookyHash::Hash64((const void*)fstd::language::to_utf8(function_node->name.text), fstd::language::get_string_size(function_node->name.text), 0);
		uint16_t short_hash = hash & 0xffff;
		AST_Node* value = (AST_Node*)function_node;

		fstd::container::hash_table_insert(globals.parser_data.current_symbol_table->functions, short_hash, function_node->name.text, value);
	};

	auto insert_parameters_to_symbol_table = [&]() {
		for (AST_Statement_Variable* argument = function_node->arguments; argument != nullptr; argument = (AST_Statement_Variable*)argument->sibling)
		{
			uint64_t hash = SpookyHash::Hash64((const void*)fstd::language::to_utf8(argument->name.text), fstd::language::get_string_size(argument->name.text), 0);
			uint16_t short_hash = hash & 0xffff;
			AST_Node* value = (AST_Node*)argument;

			fstd::container::hash_table_insert(globals.parser_data.current_symbol_table->variables, short_hash, argument->name.text, value);
		}

	};

	while (true) {
		if (current_token.type == Token_Type::SYNTAXE_OPERATOR) {
			if (current_token.value.punctuation == Punctuation::SEMICOLON) {
				stream::peek(stream); // ;
				insert_to_symbol_table();
				return;
			}
			else if (current_token.value.punctuation == Punctuation::OPEN_BRACE) {
				insert_to_symbol_table();
				push_new_symbol_table(Scope_Type::FUNCTION, &function_node->name);

				insert_parameters_to_symbol_table();

				parse_scope(stream, &function_node->scope);

				pop_symbol_table();
				return;
			}
			else if (current_token.value.punctuation == Punctuation::COLON) {
				stream::peek(stream); // :

				current_token = stream::get(stream);

				AST_Function_Modifier** current_modifier = &function_node->modifiers;
				while (!(current_token.type == Token_Type::SYNTAXE_OPERATOR
					&& (current_token.value.punctuation == Punctuation::SEMICOLON
						|| current_token.value.punctuation == Punctuation::OPEN_BRACE)))
				{
					if (current_token.type == Token_Type::IDENTIFIER)
					{
						AST_Function_Modifier* modifier_node = allocate_AST_node<AST_Function_Modifier>((AST_Node**)current_modifier);

						modifier_node->ast_type = Node_Type::STATEMENT_IDENTIFIER;
						modifier_node->sibling = nullptr;
						modifier_node->value = current_token;
						modifier_node->arguments = nullptr;

						stream::peek(stream); // identifier
						current_token = stream::get(stream);

						if (current_token.type == Token_Type::SYNTAXE_OPERATOR &&
							current_token.value.punctuation == Punctuation::COMMA) {
							stream::peek(stream); // ,
							current_token = stream::get(stream);
						}
						else if (current_token.type == Token_Type::SYNTAXE_OPERATOR &&
							current_token.value.punctuation == Punctuation::OPEN_PARENTHESIS) {

							stream::peek(stream); // (
							current_token = stream::get(stream);

							AST_Literal** current_modifier_argument = &modifier_node->arguments;
							while (!(current_token.type == Token_Type::SYNTAXE_OPERATOR
								&& current_token.value.punctuation == Punctuation::CLOSE_PARENTHESIS)) {

								if (is_literal(current_token.type)) {
									AST_Literal* modifier_argument_node = allocate_AST_node<AST_Literal>((AST_Node**)current_modifier_argument);

									modifier_argument_node->ast_type = Node_Type::STATEMENT_LITERAL;
									modifier_argument_node->sibling = nullptr;
									modifier_argument_node->value = current_token;

									stream::peek(stream); // Literal
									current_token = stream::get(stream);
								}
								else {
									// @TODO should we support compile-time expression? that generate a litteral?
									report_error(Compiler_Error::error, current_token, "Function modifiers only support literals types as arguments.");
								}

								current_modifier_argument = (AST_Literal**)&((*current_modifier_argument)->sibling);
							}

							stream::peek(stream); // )
							current_token = stream::get(stream);
						}
					}
					else {
						report_error(Compiler_Error::error, current_token, "Expecting an identifier for a modifier of function. It can take parameters as a list of litterals in enclosing parenthesis.");
					}

					current_modifier = (AST_Function_Modifier**)&((*current_modifier)->sibling);
				}
			}
		}
		else {
			report_error(Compiler_Error::error, current_token, "Expecting '->' to specify the return type of the function or the scope of the function.");
		}
	}
}

void parse_function_call(stream::Array_Read_Stream<Token<Keyword>>& stream, Token<Keyword>& identifier, AST_Function_Call** emplace_node)
{
	ZoneScopedN("parse_function_call");

	Token<Keyword>				current_token;
	AST_Function_Call*	function_call;
	AST_Node**			current_expression_node;

	current_token = stream::get(stream);
	fstd::core::Assert(current_token.type == Token_Type::SYNTAXE_OPERATOR && current_token.value.punctuation == Punctuation::OPEN_PARENTHESIS);

	function_call = allocate_AST_node<AST_Function_Call>(nullptr);
	function_call->ast_type = Node_Type::FUNCTION_CALL;
	function_call->sibling = nullptr;
	function_call->name = identifier;
	function_call->nb_arguments = 0;

	// @TODO add the check of eof with the error message
	current_expression_node = &function_call->parameters;
	while (!(current_token.type == Token_Type::SYNTAXE_OPERATOR
		&& current_token.value.punctuation == Punctuation::CLOSE_PARENTHESIS))
	{
		stream::peek(stream); // ( or ,
		parse_expression(stream, (AST_Node**)current_expression_node, Punctuation::COMMA, Punctuation::CLOSE_PARENTHESIS);
		current_token = stream::get(stream);
		function_call->nb_arguments++;
		current_expression_node = (AST_Node**)&(*current_expression_node)->sibling;
	}
	stream::peek(stream); // )

	*emplace_node = function_call;
}

void parse_struct(stream::Array_Read_Stream<Token<Keyword>>& stream, Token<Keyword>* identifier, AST_Node** previous_sibling_addr)
{
	ZoneScopedN("parse_struct");

	Token<Keyword>						current_token;
	AST_Statement_Struct_Type*	struct_node = allocate_AST_node<AST_Statement_Struct_Type>(previous_sibling_addr);

	current_token = stream::get(stream); // {

	if (!(current_token.type == Token_Type::SYNTAXE_OPERATOR && current_token.value.punctuation == Punctuation::OPEN_BRACE)) {
		report_error(Compiler_Error::error, current_token, "Expecting '{'.");
	}

	struct_node->anonymous = identifier == nullptr;
	if (identifier) {
		struct_node->name = *identifier;

		push_new_symbol_table(Scope_Type::STRUCT, &struct_node->name);
	}
	else {
		push_new_symbol_table(Scope_Type::STRUCT, nullptr);
	}

	parse_struct_scope(stream, struct_node, true);

	pop_symbol_table();

	struct_node->anonymous = identifier == nullptr;
	if (identifier)
		struct_node->name = *identifier;

	// symbol table
	if (!struct_node->anonymous)
	{
		uint64_t hash = SpookyHash::Hash64((const void*)fstd::language::to_utf8(struct_node->name.text), fstd::language::get_string_size(struct_node->name.text), 0);
		uint16_t short_hash = hash & 0xffff;
		AST_Node* value = (AST_Node*)struct_node;

		fstd::container::hash_table_insert(globals.parser_data.current_symbol_table->user_types, short_hash, struct_node->name.text, value);
	}
}

void parse_enum(stream::Array_Read_Stream<Token<Keyword>>& stream, Token<Keyword>& identifier, AST_Node** previous_sibling_addr)
{
	core::Assert(false);
}

void parse_union(stream::Array_Read_Stream<Token<Keyword>>& stream, Token<Keyword>* identifier, AST_Node** previous_sibling_addr)
{
	ZoneScopedN("parse_union");

	Token<Keyword>						current_token;
	AST_Statement_Union_Type*	union_node = allocate_AST_node<AST_Statement_Union_Type>(previous_sibling_addr);

	current_token = stream::get(stream); // {

	if (!(current_token.type == Token_Type::SYNTAXE_OPERATOR && current_token.value.punctuation == Punctuation::OPEN_BRACE)) {
		report_error(Compiler_Error::error, current_token, "Expecting '{'.");
	}

	union_node->anonymous = identifier == nullptr;
	if (identifier) {
		union_node->name = *identifier;

		push_new_symbol_table(Scope_Type::UNION, &union_node->name);
	}
	else {
		push_new_symbol_table(Scope_Type::UNION, nullptr);
	}

	parse_union_scope(stream, union_node, true);

	pop_symbol_table();

	// symbol table
	if (!union_node->anonymous)
	{
		uint64_t hash = SpookyHash::Hash64((const void*)fstd::language::to_utf8(union_node->name.text), fstd::language::get_string_size(union_node->name.text), 0);
		uint16_t short_hash = hash & 0xffff;
		AST_Node* value = (AST_Node*)union_node;

		fstd::container::hash_table_insert(globals.parser_data.current_symbol_table->user_types, short_hash, union_node->name.text, value);
	}
}

void parse_binary_operator(stream::Array_Read_Stream<Token<Keyword>>& stream, AST_Node** emplace_node, Node_Type node_type, AST_Node** previous_child, Punctuation delimiter_1, Punctuation delimiter_2)
{
	ZoneScopedN("parse_binary_operator");

	core::Assert(*previous_child != nullptr);
	AST_Binary_Operator* binary_operator_node = allocate_AST_node<AST_Binary_Operator>(emplace_node);

	binary_operator_node->ast_type = node_type;
	binary_operator_node->sibling = nullptr;
	binary_operator_node->token = stream::get(stream);
	binary_operator_node->left = *previous_child;
	binary_operator_node->right = nullptr;

	*previous_child = (AST_Node*)binary_operator_node;
	stream::peek(stream); // the binary operator

	bool scoped_by_parenthesis = parse_expression(stream, &binary_operator_node->right, delimiter_1, delimiter_2);

	// @TODO may want to take a look at precedence climbing:
	// https://eli.thegreenplace.net/2012/08/02/parsing-expressions-by-precedence-climbing
	// Is it already what I do???
	if (scoped_by_parenthesis == false) {
		fix_operations_order(binary_operator_node);
	}
}

void fix_operations_order(AST_Binary_Operator* binary_operator_node)
{
	ZoneScopedN("fix_operations_order");

	// https://ryanfleury.net/blog_a_custom_scripting_language_1
	if (is_binary_operator(binary_operator_node->right) == false)
		return;

	AST_Binary_Operator* right_node = (AST_Binary_Operator*)binary_operator_node->right;

	if (does_left_operator_precedeed_right(binary_operator_node->ast_type, right_node->ast_type) == false)
		return;

	// @TODO Handle parenthesis by starting a new sub-expression? but we want to simplify it, so maybe by setting a flag on the node

	// Step 1: Swap operator types
	{
		Node_Type right_operator_node_type = right_node->ast_type;
		Token<Keyword> right_operator_token = right_node->token;

		right_node->ast_type = binary_operator_node->ast_type;
		right_node->token = binary_operator_node->token;

		binary_operator_node->ast_type = right_operator_node_type;
		binary_operator_node->token = right_operator_token;
	}

	// Step 2: Rotate the three operands in counter-clockwise way
	{
		AST_Node* temp_node = binary_operator_node->left;

		binary_operator_node->left = right_node->right;
		right_node->right = right_node->left;
		right_node->left = temp_node;
	}

	// Step 3: Swap the original binary operator's left and right children
	{
		AST_Node* temp_node = binary_operator_node->left;

		binary_operator_node->left = (AST_Node*)right_node;
		binary_operator_node->right = temp_node;
	}
}

void parse_unary_operator(stream::Array_Read_Stream<Token<Keyword>>& stream, AST_Node** emplace_node, Node_Type node_type, AST_Node** previous_child, Punctuation delimiter_1, Punctuation delimiter_2)
{
	ZoneScopedN("parse_unary_operator");

	core::Assert(*previous_child == nullptr);
	AST_Unary_operator* unary_operator_node = allocate_AST_node<AST_Unary_operator>(emplace_node);

	unary_operator_node->ast_type = node_type;
	unary_operator_node->sibling = nullptr;
	unary_operator_node->right = nullptr;

	*previous_child = (AST_Node*)unary_operator_node;
	stream::peek(stream); // the unary operator

	parse_expression(stream, &unary_operator_node->right, delimiter_1, delimiter_2);
}

bool parse_expression(stream::Array_Read_Stream<Token<Keyword>>& stream, AST_Node** emplace_node, Punctuation delimiter_1, Punctuation delimiter_2 /* = Punctuation::UNKNOWN */)
{
	ZoneScopedN("parse_expression");

	Token<Keyword>			current_token;
	Token<Keyword>			starting_token;
	AST_Node*		previous_child = nullptr;
	bool			scoped_by_parenthesis = false;

	starting_token = stream::get(stream);

	while (stream::is_eof(stream) == false)
	{
		current_token = stream::get(stream);

		// @TODO
		// Add a method to switch node that will be used when discovering binary operators
		// And finaly add an other method that reorder the expression sub-tree depending of the operators priorities (this method will be called just before the return, use the defer?).

		if (current_token.type == Token_Type::SYNTAXE_OPERATOR) {
			if (current_token.value.punctuation == delimiter_1 || current_token.value.punctuation == delimiter_2) {
				// The delimiter will be peek be the caller
				return scoped_by_parenthesis;
			}
			else if (current_token.value.punctuation == Punctuation::OPEN_PARENTHESIS)
			{
				stream::peek(stream); // (
				parse_expression(stream, emplace_node, delimiter_1, delimiter_1);
				scoped_by_parenthesis = true;
			}
			else if (current_token.value.punctuation == Punctuation::CLOSE_PARENTHESIS)
			{
				stream::peek(stream); // )
				return scoped_by_parenthesis;
			}
			else if (current_token.value.punctuation == Punctuation::STAR) {
				parse_binary_operator(stream, emplace_node, Node_Type::BINARY_OPERATOR_MULTIPLICATION, &previous_child, delimiter_1, delimiter_2);
			}
			else if (current_token.value.punctuation == Punctuation::SLASH) {
				parse_binary_operator(stream, emplace_node, Node_Type::BINARY_OPERATOR_DIVISION, &previous_child, delimiter_1, delimiter_2);
			}
			else if (current_token.value.punctuation == Punctuation::PERCENT) {
				parse_binary_operator(stream, emplace_node, Node_Type::BINARY_OPERATOR_REMINDER, &previous_child, delimiter_1, delimiter_2);
			}
			else if (current_token.value.punctuation == Punctuation::PLUS) {
				parse_binary_operator(stream, emplace_node, Node_Type::BINARY_OPERATOR_ADDITION, &previous_child, delimiter_1, delimiter_2);
			}
			else if (current_token.value.punctuation == Punctuation::DASH) {
				if (previous_child)
					parse_binary_operator(stream, emplace_node, Node_Type::BINARY_OPERATOR_SUBSTRACTION, &previous_child, delimiter_1, delimiter_2);
				else 
					parse_unary_operator(stream, emplace_node, Node_Type::UNARY_OPERATOR_NEGATIVE, &previous_child, delimiter_1, delimiter_2);
			}
			else if (current_token.value.punctuation == Punctuation::SECTION) {
				parse_unary_operator(stream, emplace_node, Node_Type::UNARY_OPERATOR_ADDRESS_OF, &previous_child, delimiter_1, delimiter_2);
			}
			else if (current_token.value.punctuation == Punctuation::DOT) {
				parse_binary_operator(stream, emplace_node, Node_Type::BINARY_OPERATOR_MEMBER_ACCESS, &previous_child, delimiter_1, delimiter_2);
			}
			// @TODO add other arithmetic operators (bits operations,...)

			// @TODO handle pointer symbol § for alias
		}
		else if (is_literal(current_token.type))
		{
			AST_Literal* literal_node = allocate_AST_node<AST_Literal>(emplace_node);

			literal_node->ast_type = Node_Type::STATEMENT_LITERAL;
			literal_node->sibling = nullptr;
			literal_node->value = current_token;

			previous_child = (AST_Node*)literal_node;
			stream::peek(stream);
		}
		else if (current_token.type == Token_Type::IDENTIFIER)
		{
			Token<Keyword>	identifier = current_token;

			stream::peek(stream); // identifier (current_token)
			current_token = stream::get(stream);

			if (current_token.type == Token_Type::SYNTAXE_OPERATOR
				&& current_token.value.punctuation == Punctuation::OPEN_PARENTHESIS) {
				parse_function_call(stream, identifier, (AST_Function_Call**)emplace_node);
			}
			else {
				// A variable name
				AST_Identifier* identifier_node;
				if (is_binary_operator(previous_child)) {
					identifier_node = allocate_AST_node<AST_Identifier>(&((AST_Binary_Operator*)previous_child)->right);
				}
				else {
					identifier_node = allocate_AST_node<AST_Identifier>(emplace_node);
				}

				identifier_node->ast_type = Node_Type::STATEMENT_IDENTIFIER;
				identifier_node->sibling = nullptr;
				identifier_node->value = identifier;
				identifier_node->symbol_table = globals.parser_data.current_symbol_table;

				previous_child = (AST_Node*)identifier_node;
				// Identifier was already peeked
			}
	
			// followed by parenthesis it's a function call
			// followed by brackets it's an array accessor (@TODO take care of multiple arrays)
		}
		else if (current_token.type == Token_Type::KEYWORD)
		{
			if (is_a_basic_type(current_token.value.keyword)) {
				AST_Statement_Basic_Type*	basic_type_node = allocate_AST_node<AST_Statement_Basic_Type>(emplace_node);

				basic_type_node->ast_type = Node_Type::STATEMENT_BASIC_TYPE;
				basic_type_node->sibling = nullptr;
				basic_type_node->keyword = current_token.value.keyword;
				basic_type_node->token = current_token;

				previous_child = (AST_Node*)basic_type_node;
				stream::peek(stream);
			}
		}
	}

	report_error(Compiler_Error::error, starting_token, "The current expression reach the End Of File."); // @TODO add expected delimiters in the message

	// Identifier followed by ( is a function call
	// Identifier followed by [ is an array fetch
	// Identifier followed by an binary operator is a left part of a binary operation

	// Delimiters
	// In enum, or function parameters: , )
	// In scope: ;
	// In array delaclaration: ]
	// In initialization list: }
	return scoped_by_parenthesis;
}

void parse_scope(stream::Array_Read_Stream<Token<Keyword>>& stream, AST_Statement_Scope** scope_node_, bool is_root_node /* = false */)
{
	ZoneScopedN("parse_scope");

	Token<Keyword>			current_token;
	AST_Statement_Scope*	scope_node = allocate_AST_node<AST_Statement_Scope>(nullptr);
	AST_Node**				current_child = &scope_node->first_child;

	*scope_node_ = scope_node;
	scope_node->ast_type = Node_Type::STATEMENT_SCOPE;
	scope_node->sibling = nullptr;
	scope_node->first_child = nullptr;

	current_token = stream::get(stream);

	if (is_root_node == false) {
		core::Assert(current_token.type == Token_Type::SYNTAXE_OPERATOR && current_token.value.punctuation == Punctuation::OPEN_BRACE);

		stream::peek(stream); // {
	}

	while (stream::is_eof(stream) == false)
	{
		current_token = stream::get(stream);

		if (current_token.type == Token_Type::KEYWORD) {
			if (current_token.value.keyword == Keyword::IMPORT) {
				stream::peek<Token<Keyword>>(stream);
				// @TODO implement
				core::Assert(false);
			}
			else
			{
				report_error(Compiler_Error::error, current_token, "Unexpected keyword in the current context (global scope).");
			}
		}
		else if (current_token.type == Token_Type::IDENTIFIER) {
			// At global scope we can only have variable or function declarations that start with an identifier
			// @TODO We should check at which level we are

			Token<Keyword>	identifier = current_token;

			stream::peek<Token<Keyword>>(stream);
			current_token = stream::get(stream);

			if (current_token.type == Token_Type::SYNTAXE_OPERATOR) {
				if (current_token.value.punctuation == Punctuation::DOUBLE_COLON) { // It's a function, struct, union, enum or an alias declaration
					stream::peek<Token<Keyword>>(stream);
					current_token = stream::get(stream);

					if (current_token.type == Token_Type::KEYWORD) {
						if (current_token.value.keyword == Keyword::ENUM) {
							stream::peek(stream); // enum

							parse_enum(stream, identifier, current_child);
							current_child = &(*current_child)->sibling;
						}
						else if (current_token.value.keyword == Keyword::STRUCT) {
							stream::peek(stream); // struct

							parse_struct(stream, &identifier, current_child);
							current_child = &(*current_child)->sibling;
						}
						else if (current_token.value.keyword == Keyword::UNION) {
							stream::peek(stream); // union

							parse_union(stream, &identifier, current_child);
							current_child = &(*current_child)->sibling;
						}
						else if (is_a_basic_type(current_token.value.keyword)) {
							parse_alias(stream, identifier, current_child);
							current_child = &(*current_child)->sibling;
						}
					}
					else if (current_token.type == Token_Type::SYNTAXE_OPERATOR
						&& current_token.value.punctuation == Punctuation::OPEN_PARENTHESIS) {
						stream::peek(stream); // (

						parse_function(stream, identifier, current_child);
						current_child = &(*current_child)->sibling;
					}
					else if (current_token.type == Token_Type::IDENTIFIER
						|| (current_token.type == Token_Type::SYNTAXE_OPERATOR && current_token.value.punctuation == Punctuation::SECTION)) {
						parse_alias(stream, identifier, current_child); // Alias on custom type
						current_child = &(*current_child)->sibling;
					}
					else {
						report_error(Compiler_Error::error, current_token, "Expecting struct, enum, union, function signature or a type (alias declaration) after the '::' token.");
					}
				}
				else if (current_token.value.punctuation == Punctuation::COLON) { // It's a variable declaration with type
					parse_variable(stream, identifier, (AST_Statement_Variable**)current_child);
					current_child = &(*current_child)->sibling;
				}
				else if (current_token.value.punctuation == Punctuation::COLON_EQUAL) { // It's a variable declaration where type is infered

				}
				else if (current_token.value.punctuation == Punctuation::EQUALS) { // It's a variable assignement
				}
				else if (current_token.value.punctuation == Punctuation::OPEN_PARENTHESIS) {	// Function call
					// @TODO Should we need to call parse_expression?
					// but is it really valid to call a function to do an operation with assigning the result to a variable?

					parse_function_call(stream, identifier, (AST_Function_Call**)current_child);
					current_child = &(*current_child)->sibling;
					stream::peek(stream); // ;
				}
			}
		}
		else if (current_token.type == Token_Type::SYNTAXE_OPERATOR) {
			if (is_root_node == false
				&& current_token.value.punctuation == Punctuation::CLOSE_BRACE) {
				stream::peek(stream);
				return;
			}
			else if (current_token.value.punctuation == Punctuation::OPEN_BRACE) {
				push_new_symbol_table(Scope_Type::SCOPE, nullptr);

				parse_scope(stream, (AST_Statement_Scope**)current_child);
				current_child = &(*current_child)->sibling;	// Move the current_child to the sibling

				pop_symbol_table();
			}
		}
		else if (is_literal(current_token.type)) {
			report_error(Compiler_Error::error, current_token, "Expecting an expression not a literal!\n");
		}
		else if (current_token.type == Token_Type::UNKNOWN) {
			report_error(Compiler_Error::error, current_token, "Unknown token type! Something goes really badly here!\n");
		}
	}
}

void parse_struct_scope(stream::Array_Read_Stream<Token<Keyword>>& stream, AST_Statement_Struct_Type* scope_node, bool is_root_node /* = false */)
{
	ZoneScopedN("parse_struct_scope");

	Token<Keyword>						current_token;
	AST_Node**					current_child = &scope_node->first_child;

	scope_node->ast_type = Node_Type::STATEMENT_TYPE_STRUCT;
	scope_node->sibling = nullptr;
	scope_node->first_child = nullptr;

	current_token = stream::get(stream);

	core::Assert(current_token.type == Token_Type::SYNTAXE_OPERATOR && current_token.value.punctuation == Punctuation::OPEN_BRACE);

	stream::peek(stream); // {

	while (stream::is_eof(stream) == false)
	{
		current_token = stream::get(stream);

		if (current_token.type == Token_Type::KEYWORD) {
			if (current_token.value.keyword == Keyword::IMPORT) {
				stream::peek<Token<Keyword>>(stream);
				// @TODO implement
				core::Assert(false);
			}
			else
			{
				report_error(Compiler_Error::error, current_token, "Unexpected keyword in the current context (global scope).");
			}
		}
		else if (current_token.type == Token_Type::IDENTIFIER) {
			// At global scope we can only have variable or function declarations that start with an identifier
			// @TODO We should check at which level we are

			Token<Keyword>	identifier = current_token;

			stream::peek<Token<Keyword>>(stream);
			current_token = stream::get(stream);

			if (current_token.type == Token_Type::SYNTAXE_OPERATOR) {
				if (current_token.value.punctuation == Punctuation::DOUBLE_COLON) { // It's a function, struct, union, enum or an alias declaration
					stream::peek<Token<Keyword>>(stream);
					current_token = stream::get(stream);

					if (current_token.type == Token_Type::KEYWORD) {
						if (current_token.value.keyword == Keyword::ENUM) {
							stream::peek(stream); // enum

							parse_enum(stream, identifier, current_child);
							current_child = &(*current_child)->sibling;
						}
						else if (current_token.value.keyword == Keyword::STRUCT) {
							stream::peek(stream); // struct

							parse_struct(stream, &identifier, current_child);
							current_child = &(*current_child)->sibling;
						}
						else if (current_token.value.keyword == Keyword::UNION) {
							stream::peek(stream); // union

							parse_union(stream, &identifier, current_child);
							current_child = &(*current_child)->sibling;
						}
						else if (is_a_basic_type(current_token.value.keyword)) {
							parse_alias(stream, identifier, current_child);
							current_child = &(*current_child)->sibling;
						}
					}
					else {
						report_error(Compiler_Error::error, current_token, "Expecting struct, enum or union");
					}
				}
				else if (current_token.value.punctuation == Punctuation::COLON) { // It's a variable declaration with type
					parse_variable(stream, identifier, (AST_Statement_Variable**)current_child);
					current_child = &(*current_child)->sibling;
				}
				else if (current_token.value.punctuation == Punctuation::COLON_EQUAL) { // It's a variable declaration where type is infered

				}
				else if (current_token.value.punctuation == Punctuation::EQUALS) { // It's a variable assignement
				}
			}
		}
		else if (current_token.type == Token_Type::SYNTAXE_OPERATOR) {
			if (current_token.value.punctuation == Punctuation::CLOSE_BRACE) { // Ends struct declaration
				stream::peek(stream); // }
				return;
			}
			else {
				report_error(Compiler_Error::error, current_token, "Unnexpected syntaxe operator");
			}
		}
		else if (is_literal(current_token.type)) {
			report_error(Compiler_Error::error, current_token, "Expecting an expression not a literal!\n");
		}
		else if (current_token.type == Token_Type::UNKNOWN) {
			report_error(Compiler_Error::error, current_token, "Unknown token type! Something goes really badly here!\n");
		}
	}
}

void parse_union_scope(stream::Array_Read_Stream<Token<Keyword>>& stream, AST_Statement_Union_Type* scope_node, bool is_root_node /* = false */)
{
	ZoneScopedN("parse_union_scope");

	Token<Keyword>						current_token;
	AST_Node**					current_child = &scope_node->first_child;

	scope_node->ast_type = Node_Type::STATEMENT_TYPE_UNION;
	scope_node->sibling = nullptr;
	scope_node->first_child = nullptr;

	current_token = stream::get(stream);

	core::Assert(current_token.type == Token_Type::SYNTAXE_OPERATOR && current_token.value.punctuation == Punctuation::OPEN_BRACE);

	stream::peek(stream); // {

	while (stream::is_eof(stream) == false)
	{
		current_token = stream::get(stream);

		if (current_token.type == Token_Type::KEYWORD) {
			if (current_token.value.keyword == Keyword::IMPORT) {
				stream::peek<Token<Keyword>>(stream);
				// @TODO implement
				core::Assert(false);
			}
			else
			{
				report_error(Compiler_Error::error, current_token, "Unexpected keyword in the current context (global scope).");
			}
		}
		else if (current_token.type == Token_Type::IDENTIFIER) {
			// At global scope we can only have variable or function declarations that start with an identifier
			// @TODO We should check at which level we are

			Token<Keyword>	identifier = current_token;

			stream::peek<Token<Keyword>>(stream);
			current_token = stream::get(stream);

			if (current_token.type == Token_Type::SYNTAXE_OPERATOR) {
				if (current_token.value.punctuation == Punctuation::DOUBLE_COLON) { // It's a function, struct, union, enum or an alias declaration
					stream::peek<Token<Keyword>>(stream);
					current_token = stream::get(stream);

					if (current_token.type == Token_Type::KEYWORD) {
						if (current_token.value.keyword == Keyword::ENUM) {
							stream::peek(stream); // enum

							parse_enum(stream, identifier, current_child);
							current_child = &(*current_child)->sibling;
						}
						else if (current_token.value.keyword == Keyword::STRUCT) {
							stream::peek(stream); // struct

							parse_struct(stream, &identifier, current_child);
							current_child = &(*current_child)->sibling;
						}
						else if (current_token.value.keyword == Keyword::UNION) {
							stream::peek(stream); // union

							parse_union(stream, &identifier, current_child);
							current_child = &(*current_child)->sibling;
						}
						else if (is_a_basic_type(current_token.value.keyword)) {
							parse_alias(stream, identifier, current_child);
							current_child = &(*current_child)->sibling;
						}
					}
					else {
						report_error(Compiler_Error::error, current_token, "Expecting struct, enum or union");
					}
				}
				else if (current_token.value.punctuation == Punctuation::COLON) { // It's a variable declaration with type
					parse_variable(stream, identifier, (AST_Statement_Variable**)current_child);
					current_child = &(*current_child)->sibling;
				}
				else if (current_token.value.punctuation == Punctuation::COLON_EQUAL) { // It's a variable declaration where type is infered

				}
				else if (current_token.value.punctuation == Punctuation::EQUALS) { // It's a variable assignement
				}
			}
		}
		else if (current_token.type == Token_Type::SYNTAXE_OPERATOR) {
			if (current_token.value.punctuation == Punctuation::CLOSE_BRACE) { // Ends union declaration
				stream::peek(stream); // }
				return;
			}
			else {
				report_error(Compiler_Error::error, current_token, "Unnexpected syntaxe operator");
			}
		}
		else if (is_literal(current_token.type)) {
			report_error(Compiler_Error::error, current_token, "Expecting an expression not a literal!\n");
		}
		else if (current_token.type == Token_Type::UNKNOWN) {
			report_error(Compiler_Error::error, current_token, "Unknown token type! Something goes really badly here!\n");
		}
	}
}

void initialize_symbol_table(Symbol_Table* symbol_table, Symbol_Table* parent, Symbol_Table* sibling, Scope_Type type, Token<Keyword>* name)
{
	ZoneScopedN("initialize_symbol_table");

	fstd::container::hash_table_init(symbol_table->variables,	&fstd::language::are_equals);
	fstd::container::hash_table_init(symbol_table->user_types,	&fstd::language::are_equals);
	fstd::container::hash_table_init(symbol_table->functions,	&fstd::language::are_equals);

	symbol_table->type = type;
	symbol_table->name = name;
	symbol_table->parent = parent;
	symbol_table->sibling = sibling;
	symbol_table->first_child = nullptr;
}

void f::parse(fstd::container::Array<Token<Keyword>>& tokens, Parsing_Result& parsing_result)
{
	ZoneScopedNC("f::parse", 0xff6f00);

	stream::Array_Read_Stream<Token<Keyword>>	stream;

	// It is impossible to have more nodes than tokens, so we can easily pre-allocate them to the maximum possible size.
	// maximum_size = nb_tokens * largest_node_size
	//
	// @TODO
	// We expect to be able to determine with the meta-programmation which AST_Node type is the largest one.
	//
	// Flamaros - 13 april 2020
	container::reserve_array(globals.parser_data.ast_nodes, container::get_array_size(tokens) * sizeof(AST_Statement_Variable));
	container::reserve_array(globals.parser_data.symbol_tables, container::get_array_size(tokens) * sizeof(Symbol_Table));

	stream::init<Token<Keyword>>(stream, tokens);

	if (stream::is_eof(stream) == true) {
		return;
	}

	push_new_symbol_table(Scope_Type::MODULE, nullptr);
	parsing_result.symbol_table_root = globals.parser_data.current_symbol_table;
	parse_scope(stream, (AST_Statement_Scope**)&parsing_result.ast_root, true);
}

static void write_dot_node(String_Builder& file_string_builder, const AST_Node* node, int64_t parent_index = -1, int64_t left_node_index = -1)
{
	ZoneScopedN("write_dot_node");

	if (!node) {
		return;
	}

	language::string	dot_node;
	static uint32_t		nb_nodes = 0;
	uint32_t			node_index = nb_nodes++;

	defer{
		release(dot_node);
	};

	if (parent_index != -1) {
		if (left_node_index == -1) {
			print_to_builder(file_string_builder, "\t" "node_%ld -> node_%ld\n", parent_index, node_index);
		}
		else {
			print_to_builder(file_string_builder, "\t" "node_%ld -> node_%ld [color=\"dodgerblue\"]\n", parent_index, node_index);
		}
	}

	// @TODO Here we could make an arrow between siblings, but we have to use subgraph to make the nodes stay at the right position.
	// Take a look at:
	// https://stackoverflow.com/questions/3322827/how-to-set-fixed-depth-levels-in-dot-graphs
	//
	// Maybe using colors is enough to ease the understanding of links
	//
	// Flamaros - 09 may 2020

	print_to_builder(file_string_builder, "\n\t" "node_%ld [label=\"", node_index);
	if (node->ast_type == Node_Type::TYPE_ALIAS) {
		AST_Alias* alias_node = (AST_Alias*)node;

		print_to_builder(file_string_builder,
			"%Cv"
			"\n%v", magic_enum::enum_name(node->ast_type), alias_node->name.text);
	}
	else if (node->ast_type == Node_Type::STATEMENT_BASIC_TYPE) {
		AST_Statement_Basic_Type*	basic_type_node = (AST_Statement_Basic_Type*)node;

		print_to_builder(file_string_builder,
			"%Cv"
			"\n%Cv", magic_enum::enum_name(node->ast_type), magic_enum::enum_name(basic_type_node->keyword));
	}
	else if (node->ast_type == Node_Type::USER_TYPE_IDENTIFIER) {
		AST_User_Type_Identifier*	user_type_node = (AST_User_Type_Identifier*)node;

		print_to_builder(file_string_builder,
			"%Cv"
			"\n%v", magic_enum::enum_name(node->ast_type), user_type_node->identifier.text);
	}
	else if (node->ast_type == Node_Type::STATEMENT_TYPE_POINTER) {
		AST_Statement_Type_Pointer*	basic_type_node = (AST_Statement_Type_Pointer*)node;

		print_to_builder(file_string_builder,
			"%Cv", magic_enum::enum_name(node->ast_type));
	}
	else if (node->ast_type == Node_Type::STATEMENT_FUNCTION) {
		AST_Statement_Function*	function_node = (AST_Statement_Function*)node;

		print_to_builder(file_string_builder,
			"%Cv"
			"\nname: %v (nb_arguments: %d)", magic_enum::enum_name(node->ast_type), function_node->name.text, function_node->nb_arguments);
	}
	else if (node->ast_type == Node_Type::STATEMENT_VARIABLE) {
		AST_Statement_Variable*	variable_node = (AST_Statement_Variable*)node;

		print_to_builder(file_string_builder,
			"%Cv"
			"\nname: %v (is_parameter: %d is_optional: %d)", magic_enum::enum_name(node->ast_type), variable_node->name.text, variable_node->is_function_parameter, variable_node->is_optional);
	}
	else if (node->ast_type == Node_Type::STATEMENT_TYPE_ARRAY) {
		AST_Statement_Type_Array*	array_node = (AST_Statement_Type_Array*)node;

		print_to_builder(file_string_builder,
			"%Cv", magic_enum::enum_name(node->ast_type));
	}
	else if (node->ast_type == Node_Type::STATEMENT_SCOPE) {
		AST_Statement_Scope*	scope_node = (AST_Statement_Scope*)node;

		print_to_builder(file_string_builder,
			"%Cv", magic_enum::enum_name(node->ast_type));
	}
	else if (node->ast_type == Node_Type::STATEMENT_LITERAL) {
		AST_Literal* literal_node = (AST_Literal*)node;

		if (literal_node->value.type == Token_Type::STRING_LITERAL) {
			print_to_builder(file_string_builder,
				"%Cv"
				"\n%v", magic_enum::enum_name(node->ast_type), literal_node->value.text);
			// @TODO use the *literal_node->value.value.string instead of the token's text
		}
		else if (literal_node->value.type == Token_Type::NUMERIC_LITERAL_I32
			|| literal_node->value.type == Token_Type::NUMERIC_LITERAL_I64) {
			print_to_builder(file_string_builder,
				"%Cv"
				"\n%ld", magic_enum::enum_name(node->ast_type), literal_node->value.value.integer);
		}
		else {
			core::Assert(false);
			// @TODO implement it
			// Actually the string builder doesn't support floats and unsigned intergers
		}
	}
	else if (node->ast_type == Node_Type::STATEMENT_IDENTIFIER) {
		AST_Identifier* identifier_node = (AST_Identifier*)node;

		print_to_builder(file_string_builder,
			"%Cv"
			"\n%v", magic_enum::enum_name(node->ast_type), identifier_node->value.text);
	}
	else if (node->ast_type == Node_Type::FUNCTION_CALL) {
		AST_Function_Call* function_call_node = (AST_Function_Call*)node;

		print_to_builder(file_string_builder,
			"%Cv"
			"\nname: %v (nb_arguments: %d)", magic_enum::enum_name(node->ast_type), function_call_node->name.text, function_call_node->nb_arguments);
	}
	else if (is_unary_operator(node)) {
		AST_Unary_operator* address_of_node = (AST_Unary_operator*)node;

		print_to_builder(file_string_builder,
			"%Cv", magic_enum::enum_name(node->ast_type));
	}
	else if (is_binary_operator(node)) {
		AST_Binary_Operator* member_access_node = (AST_Binary_Operator*)node;

		print_to_builder(file_string_builder,
			"%Cv", magic_enum::enum_name(node->ast_type));
	}
	else if (node->ast_type == Node_Type::STATEMENT_TYPE_STRUCT) {
	AST_Statement_Struct_Type* struct_node = (AST_Statement_Struct_Type*)node;

		if (struct_node->anonymous)
			print_to_builder(file_string_builder,
				"%Cv"
				"\nname: %Cs", magic_enum::enum_name(node->ast_type), "anonymous");
		else
			print_to_builder(file_string_builder,
				"%Cv"
				"\nname: %v", magic_enum::enum_name(node->ast_type), struct_node->name.text);
	}
	else if (node->ast_type == Node_Type::STATEMENT_TYPE_UNION) {
	AST_Statement_Union_Type* union_node = (AST_Statement_Union_Type*)node;

	if (union_node->anonymous)
		print_to_builder(file_string_builder,
			"%Cv"
			"\nname: %Cs", magic_enum::enum_name(node->ast_type), "anonymous");
	else
		print_to_builder(file_string_builder,
			"%Cv"
			"\nname: %v", magic_enum::enum_name(node->ast_type), union_node->name.text);
	}
	else {
		core::Assert(false);
		print_to_builder(file_string_builder,
			"UNKNOWN type"
			"\nnode_%ld", node_index);
	}
	print_to_builder(file_string_builder, "\" shape=box, style=filled, color=black, fillcolor=lightseagreen]\n");

	// Children iteration
	if (node->ast_type == Node_Type::TYPE_ALIAS) {
		AST_Alias*	alias_node = (AST_Alias*)node;
		write_dot_node(file_string_builder, alias_node->type, node_index);
	}
	else if (node->ast_type == Node_Type::STATEMENT_BASIC_TYPE) {
		// No children
	}
	else if (node->ast_type == Node_Type::USER_TYPE_IDENTIFIER) {
		// No children
	}
	else if (node->ast_type == Node_Type::STATEMENT_TYPE_POINTER) {
		// No children
	}
	else if (node->ast_type == Node_Type::STATEMENT_FUNCTION) {
		AST_Statement_Function*	function_node = (AST_Statement_Function*)node;
		write_dot_node(file_string_builder, (AST_Node*)function_node->arguments, node_index);
		write_dot_node(file_string_builder, function_node->return_type, node_index);
		write_dot_node(file_string_builder, (AST_Node*)function_node->scope, node_index);
	}
	else if (node->ast_type == Node_Type::STATEMENT_VARIABLE) {
		AST_Statement_Variable*	variable_node = (AST_Statement_Variable*)node;

		write_dot_node(file_string_builder, variable_node->type, node_index);
		write_dot_node(file_string_builder, (AST_Node*)variable_node->expression, node_index);
	}
	else if (node->ast_type == Node_Type::STATEMENT_TYPE_ARRAY) {
		AST_Statement_Type_Array*	array_node = (AST_Statement_Type_Array*)node;

		write_dot_node(file_string_builder, (AST_Node*)array_node->array_size, node_index);
	}
	else if (node->ast_type == Node_Type::STATEMENT_SCOPE) {
		AST_Statement_Scope* scope_node = (AST_Statement_Scope*)node;

		write_dot_node(file_string_builder, (AST_Node*)scope_node->first_child, node_index);
	}
	else if (node->ast_type == Node_Type::STATEMENT_LITERAL) {
		// No children
	}
	else if (node->ast_type == Node_Type::STATEMENT_IDENTIFIER) {
		// No children
	}
	else if (node->ast_type == Node_Type::FUNCTION_CALL) {
		AST_Function_Call* function_call_node = (AST_Function_Call*)node;

		write_dot_node(file_string_builder, (AST_Node*)function_call_node->parameters, node_index);
	}
	else if (is_unary_operator(node)) {
		AST_Unary_operator* address_of_node = (AST_Unary_operator*)node;

		write_dot_node(file_string_builder, (AST_Node*)address_of_node->right, node_index);
	}
	else if (is_binary_operator(node)) {
		AST_Binary_Operator* member_access_node = (AST_Binary_Operator*)node;

		write_dot_node(file_string_builder, (AST_Node*)member_access_node->left, node_index);
		write_dot_node(file_string_builder, (AST_Node*)member_access_node->right, node_index);
	}
	else if (node->ast_type == Node_Type::STATEMENT_TYPE_STRUCT) {
		AST_Statement_Struct_Type* struct_node = (AST_Statement_Struct_Type*)node;

		write_dot_node(file_string_builder, (AST_Node*)struct_node->first_child, node_index);
	}
	else if (node->ast_type == Node_Type::STATEMENT_TYPE_UNION) {
		AST_Statement_Union_Type* union_node = (AST_Statement_Union_Type*)node;

		write_dot_node(file_string_builder, (AST_Node*)union_node->first_child, node_index);
	}
	else {
		core::Assert(false);
	}

	// Sibling iteration
	if (node->sibling) {
		write_dot_node(file_string_builder, node->sibling, parent_index, node_index);
	}

	dot_node = to_string(file_string_builder);
}

void f::generate_dot_file(const AST_Node* node, const system::Path& output_file_path)
{
	ZoneScopedNC("f::generate_ast_dot_file", 0xc43e00);

	system::File		file;
	String_Builder		string_builder;
	language::string	file_content;

	defer {
		free_buffers(string_builder);
		release(file_content);
		close_file(file);
	};

	if (open_file(file, output_file_path, (system::File::Opening_Flag)
		((uint32_t)system::File::Opening_Flag::CREATE
			| (uint32_t)system::File::Opening_Flag::WRITE)) == false) {
		print_to_builder(string_builder, "Failed to open file: %s", to_string(output_file_path));
		system::print(to_string(string_builder));
		return;
	}

	print_to_builder(string_builder, "digraph {\n");
	print_to_builder(string_builder, "\t" "rankdir = TB\n");

	write_dot_node(string_builder, node);

	print_to_builder(string_builder, "}\n");

	file_content = to_string(string_builder);

	system::write_file(file, language::to_utf8(file_content), (uint32_t)language::get_string_size(file_content));
}

static void write_dot_symbol_table(String_Builder& file_string_builder, const Symbol_Table* symbol_table, int64_t parent_index = -1, int64_t left_node_index = -1)
{
	ZoneScopedN("write_dot_symbol_table");

	if (!symbol_table) {
		return;
	}

	language::string	dot_scope;
	static uint32_t		nb_nodes = 0;
	uint32_t			node_index = nb_nodes++;

	defer{
		release(dot_scope);
	};

	if (parent_index != -1) {
		if (left_node_index == -1) {
			print_to_builder(file_string_builder, "\t" "node_%ld -> node_%ld\n", parent_index, node_index);
		}
		else {
			print_to_builder(file_string_builder, "\t" "node_%ld -> node_%ld [color=\"dodgerblue\"]\n", parent_index, node_index);
		}
	}

	// @TODO Here we could make an arrow between siblings, but we have to use subgraph to make the nodes stay at the right position.
	// Take a look at:
	// https://stackoverflow.com/questions/3322827/how-to-set-fixed-depth-levels-in-dot-graphs
	//
	// Maybe using colors is enough to ease the understanding of links
	//
	// Flamaros - 09 may 2020
//<TABLE border="10" cellspacing="10" cellpadding="10" style="rounded" bgcolor="/rdylgn11/1:/rdylgn11/11" gradientangle="315">
	
	// Symbol table header
	{
		print_to_builder(file_string_builder, "\n\t" "node_%ld [label=<\n", node_index);
		print_to_builder(file_string_builder,
			"\t\t" "<table border=\"0\" cellborder=\"1\" cellspacing=\"0\"><tr><td colspan=\"2\">%Cv", magic_enum::enum_name(symbol_table->type));
		if (symbol_table->name) {
			print_to_builder(file_string_builder, " %v", symbol_table->name->text);
		}
		print_to_builder(file_string_builder, "</td></tr>\n");
	}

	// variables
	{
		print_to_builder(file_string_builder,
			"\t\t\t" "<tr><td colspan=\"2\"></td></tr>\n\n"
			"\t\t\t" "<tr><td colspan=\"2\">Variables</td></tr>\n");
		auto it = fstd::container::hash_table_begin(symbol_table->variables);
		auto it_end = fstd::container::hash_table_end(symbol_table->variables);

		for (; !fstd::container::equals<uint16_t, fstd::language::string_view, AST_Node*, 32>(it, it_end); fstd::container::hash_table_next<uint16_t, fstd::language::string_view, AST_Node*, 32>(it))
		{
			AST_Node* node = *fstd::container::hash_table_get<uint16_t, fstd::language::string_view, AST_Node*, 32>(it);

			if (node->ast_type == Node_Type::STATEMENT_VARIABLE) {
				AST_Statement_Variable* variable = ((AST_Statement_Variable*)node);
				if (symbol_table->type == Scope_Type::FUNCTION && variable->is_function_parameter) { // Parameters of a function declaration aren't visible for the current scope
					print_to_builder(file_string_builder, "\t\t\t\t" "<tr><td>parameter</td><td>%v</td></tr>\n", variable->name.text);
				}
				else if (!variable->is_function_parameter) {
					print_to_builder(file_string_builder, "\t\t\t\t" "<tr><td>variable</td><td>%v</td></tr>\n", variable->name.text);
				}
			}
		}
	}

	// user types
	{
		print_to_builder(file_string_builder,
			"\t\t\t" "<tr><td colspan=\"2\"></td></tr>\n\n"
			"\t\t\t" "<tr><td colspan=\"2\">User types</td></tr>\n");
		auto it = fstd::container::hash_table_begin(symbol_table->user_types);
		auto it_end = fstd::container::hash_table_end(symbol_table->user_types);

		for (; !fstd::container::equals<uint16_t, fstd::language::string_view, AST_Node*, 32>(it, it_end); fstd::container::hash_table_next<uint16_t, fstd::language::string_view, AST_Node*, 32>(it))
		{
			AST_Node* node = *fstd::container::hash_table_get<uint16_t, fstd::language::string_view, AST_Node*, 32>(it);

			if (node->ast_type == Node_Type::TYPE_ALIAS) {
				print_to_builder(file_string_builder, "\t\t\t\t" "<tr><td>alias</td><td>%v</td></tr>\n", ((AST_Alias*)node)->name.text);
			}
			else if (node->ast_type == Node_Type::STATEMENT_TYPE_STRUCT) {
				print_to_builder(file_string_builder, "\t\t\t\t" "<tr><td>struct</td><td>%v</td></tr>\n", ((AST_Statement_Struct_Type*)node)->name.text);
			}
			else if (node->ast_type == Node_Type::STATEMENT_TYPE_UNION) {
				print_to_builder(file_string_builder, "\t\t\t\t" "<tr><td>union</td><td>%v</td></tr>\n", ((AST_Statement_Union_Type*)node)->name.text);
			}
			//else if (node->ast_type == Node_Type::STATEMENT_TYPE_ENUM) {
			//	print_to_builder(file_string_builder, "<tr><td>enum</td><td>%v</td></tr>\n", ((AST_Statement_Enum_Type*)node)->name.text);
			//}
			else
			{
				assert(false);
			}
		}
	}

	// functions
	{
		print_to_builder(file_string_builder,
			"\t\t\t" "<tr><td colspan=\"2\"></td></tr>\n\n"
			"\t\t\t" "<tr><td colspan=\"2\">Functions</td></tr>\n");
		auto it = fstd::container::hash_table_begin(symbol_table->functions);
		auto it_end = fstd::container::hash_table_end(symbol_table->functions);

		for (; !fstd::container::equals<uint16_t, fstd::language::string_view, AST_Node*, 32>(it, it_end); fstd::container::hash_table_next<uint16_t, fstd::language::string_view, AST_Node*, 32>(it))
		{
			AST_Node* node = *fstd::container::hash_table_get<uint16_t, fstd::language::string_view, AST_Node*, 32>(it);

			if (node->ast_type == Node_Type::STATEMENT_FUNCTION) {
				print_to_builder(file_string_builder, "\t\t\t\t" "<tr><td>function</td><td>%v</td></tr>\n", ((AST_Statement_Function*)node)->name.text);
			}
		}
	}
	print_to_builder(file_string_builder, "\t\t" "</table>>\n"
		"\t\t" "shape=box, style=filled, color=black, fillcolor=lightseagreen]\n");

	if (symbol_table->first_child) {
		write_dot_symbol_table(file_string_builder, symbol_table->first_child, node_index);
	}

	// Sibling iteration
	if (symbol_table->sibling) {
		write_dot_symbol_table(file_string_builder, symbol_table->sibling, parent_index, node_index);
	}

	dot_scope = to_string(file_string_builder);
}

void f::generate_dot_file(const Symbol_Table* symbol_table, const system::Path& output_file_path)
{
	ZoneScopedNC("f::generate_symbol_table_dot_file", 0xc43e00);

	system::File		file;
	String_Builder		string_builder;
	language::string	file_content;

	defer{
		free_buffers(string_builder);
		release(file_content);
		close_file(file);
	};

	if (open_file(file, output_file_path, (system::File::Opening_Flag)
		((uint32_t)system::File::Opening_Flag::CREATE
			| (uint32_t)system::File::Opening_Flag::WRITE)) == false) {
		print_to_builder(string_builder, "Failed to open file: %s", to_string(output_file_path));
		system::print(to_string(string_builder));
		return;
	}

	print_to_builder(string_builder, "digraph {\n");
	print_to_builder(string_builder, "\t" "rankdir = TB\n");

	write_dot_symbol_table(string_builder, symbol_table);

	print_to_builder(string_builder, "}\n");

	file_content = to_string(string_builder);

	system::write_file(file, language::to_utf8(file_content), (uint32_t)language::get_string_size(file_content));
}
