#include "parser.hpp"

#include "globals.hpp"

#include <fstd/core/logger.hpp>
#include <fstd/core/string_builder.hpp>

#include <fstd/memory/array.hpp>

#include <fstd/stream/array_stream.hpp>

#include <fstd/system/file.hpp>
#include <fstd/system/stdio.hpp>

#include <fstd/language/defer.hpp>
#include <fstd/language/flags.hpp>
#include <fstd/language/string.hpp>

#undef max
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
	// @TODO f-lang
	// Check if Node_Type is an AST_Node at compile time
	//
	// Flamaros - 13 april 2020

	// Ensure that no reallocation could happen during the resize
	core::Assert(memory::get_array_size(globals.parser_data.ast_nodes) < memory::get_array_reserved(globals.parser_data.ast_nodes) * sizeof(AST_Statement_Variable));

	memory::resize_array(globals.parser_data.ast_nodes, memory::get_array_size(globals.parser_data.ast_nodes) + sizeof(Node_Type));

	Node_Type* new_node = (Node_Type*)(memory::get_array_last_element(globals.parser_data.ast_nodes) - sizeof(Node_Type));
	if (emplace_node) {
		*emplace_node = (AST_Node*)new_node;
	}
	return new_node;
}

// =============================================================================

static void parse_array(stream::Array_Stream<Token>& stream, AST_Statement_Type_Array** array_node_);
static void parse_type(stream::Array_Stream<Token>& stream, AST_Node** type_node);
static void parse_variable(stream::Array_Stream<Token>& stream, Token& identifier, AST_Statement_Variable** variable_, bool is_function_parameter = false);
static void parse_function(stream::Array_Stream<Token>& stream, Token& identifier, AST_Node** previous_sibling_addr);
static void parse_function_call(stream::Array_Stream<Token>& stream, Token& identifier, AST_Function_Call** emplace_node);
static void parse_struct(stream::Array_Stream<Token>& stream, Token& identifier, AST_Node** previous_sibling_addr);
static void parse_enum(stream::Array_Stream<Token>& stream, Token& identifier, AST_Node** previous_sibling_addr);
static void parse_expression(stream::Array_Stream<Token>& stream, AST_Node** emplace_node, bool is_sub_expression, Punctuation delimiter_1, Punctuation delimiter_2 = Punctuation::UNKNOWN);
static void parse_scope(stream::Array_Stream<Token>& stream, AST_Statement_Scope** scope_node_, bool is_root_node = false);

// =============================================================================

void parse_array(stream::Array_Stream<Token>& stream, AST_Statement_Type_Array** array_node_)
{
	Token	current_token;

	current_token = stream::get(stream);
	core::Assert(current_token.type == Token_Type::SYNTAXE_OPERATOR && current_token.value.punctuation == Punctuation::OPEN_BRACKET);

	AST_Statement_Type_Array*	array_node = allocate_AST_node<AST_Statement_Type_Array>(nullptr);
	array_node->ast_type = Node_Type::STATEMENT_TYPE_ARRAY;
	array_node->sibling = nullptr;
	array_node->array_size = nullptr;

	*array_node_ = array_node;

	stream::peek(stream); // [

// @TODO Implement it
	while (true)
	{
		current_token = stream::get(stream);
		if (current_token.value.punctuation == Punctuation::CLOSE_BRACKET) {
			stream::peek(stream);
			return;
		}
		stream::peek(stream);
	}
}

void parse_type(stream::Array_Stream<Token>& stream, AST_Node** type_node)
{
	Token		current_token;
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

				stream::peek(stream); // basic_type keyword
				break;
			}
			else {
				report_error(Compiler_Error::error, current_token, "Expecting a type qualifier (a type modifier operator, a basic type keyword or a user type identifier).");
			}
		}
		else if (current_token.type == Token_Type::IDENTIFIER) {
			AST_Statement_User_Type*	user_type_node = allocate_AST_node<AST_Statement_User_Type>(previous_sibling_addr);

			if (*type_node == nullptr) {
				*type_node = (AST_Node*)user_type_node;
			}

			user_type_node->ast_type = Node_Type::STATEMENT_USER_TYPE;
			user_type_node->sibling = nullptr;
			user_type_node->identifier = current_token;

			stream::peek(stream); // identifier
			break;
		}
		else {
			report_error(Compiler_Error::error, current_token, "Expecting a type qualifier (a type modifier operator, a basic type keyword or a user type identifier).");
		}
	}
}

void parse_variable(stream::Array_Stream<Token>& stream, Token& identifier, AST_Statement_Variable** variable_, bool is_function_parameter /* = false */)
{
	Token					current_token;
	AST_Statement_Variable* variable;

	current_token = stream::get(stream);
	fstd::core::Assert(current_token.type == Token_Type::SYNTAXE_OPERATOR && current_token.value.punctuation ==  Punctuation::COLON);

	variable = allocate_AST_node<AST_Statement_Variable>(nullptr);
	variable->ast_type = Node_Type::STATEMENT_VARIABLE;
	variable->sibling = nullptr;
	variable->name = identifier;
	variable->type = nullptr;
	variable->is_function_paramter = is_function_parameter;
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
			parse_expression(stream, (AST_Node**)&variable->expression, false, Punctuation::COMMA, Punctuation::CLOSE_PARENTHESIS);
		}
		else {
			parse_expression(stream, (AST_Node**)&variable->expression, true, Punctuation::SEMICOLON);
		}
	}

	if (is_function_parameter == false) {	// @Warning the parse_function method have to be able to read arguments delimiters ',' or ')' characters
		stream::peek(stream); // ;
	}
}

void parse_function(stream::Array_Stream<Token>& stream, Token& identifier, AST_Node** previous_sibling_addr)
{
	Token					current_token;
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
			Token	identifier = current_token;

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

	if (current_token.type == Token_Type::SYNTAXE_OPERATOR
		&& current_token.value.punctuation == Punctuation::SEMICOLON) {
		// @TODO implement it, this is just a function declaration in this case
		// @Warning this can also just follow the return value
		core::Assert(false);
	}
	else if (current_token.type == Token_Type::SYNTAXE_OPERATOR
		&& current_token.value.punctuation == Punctuation::OPEN_BRACE) {
		parse_scope(stream, &function_node->scope);
		current_token = stream::get(stream);
	}
	else {
		report_error(Compiler_Error::error, current_token, "Expecting '->' to specify the return type of the function or the scope of the function.");
	}
}

void parse_function_call(stream::Array_Stream<Token>& stream, Token& identifier, AST_Function_Call** emplace_node)
{
	Token				current_token;
	AST_Function_Call*	function_call;
	AST_Expression**	current_expression_node;

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
		parse_expression(stream, (AST_Node**)current_expression_node, false, Punctuation::COMMA, Punctuation::CLOSE_PARENTHESIS);
		current_token = stream::get(stream);
		function_call->nb_arguments++;
		current_expression_node = (AST_Expression**)&(*current_expression_node)->sibling;
	}
	stream::peek(stream); // )

	*emplace_node = function_call;
}

void parse_struct(stream::Array_Stream<Token>& stream, Token& identifier, AST_Node** previous_sibling_addr)
{
	core::Assert(false);
}

void parse_enum(stream::Array_Stream<Token>& stream, Token& identifier, AST_Node** previous_sibling_addr)
{
	core::Assert(false);
}

void parse_alias(stream::Array_Stream<Token>& stream, AST_Node** previous_sibling_addr)
{
	Token		current_token;
	AST_Alias*	alias_node = allocate_AST_node<AST_Alias>(previous_sibling_addr);

	alias_node->ast_type = Node_Type::TYPE_ALIAS;
	alias_node->sibling = nullptr;
	alias_node->type_name = stream::get(stream);
	alias_node->type = nullptr;
	stream::peek(stream); // alias name
	current_token = stream::get(stream);
	if (current_token.type != Token_Type::SYNTAXE_OPERATOR
		&& current_token.value.punctuation != Punctuation::EQUALS) {
		report_error(Compiler_Error::error, current_token, "Expecting '=' punctuation after the alias typename.");
	}
	stream::peek(stream); // =
	parse_type(stream, &alias_node->type);
	current_token = stream::get(stream);

	if (current_token.type != Token_Type::SYNTAXE_OPERATOR
		&& current_token.value.punctuation != Punctuation::SEMICOLON) {
		report_error(Compiler_Error::error, current_token, "Expecting ';' punctuation after at the end of the alias statement.");
	}
	stream::peek(stream); // ;
}

void parse_expression(stream::Array_Stream<Token>& stream, AST_Node** emplace_node, bool is_sub_expression, Punctuation delimiter_1, Punctuation delimiter_2 /* = Punctuation::UNKNOWN */)
{
	Token			current_token;
	Token			starting_token;
	AST_Node*		previous_child = nullptr;

	if (is_sub_expression == false) {
		// We should add an intermediate expression node
		AST_Expression* expression_node = allocate_AST_node<AST_Expression>(emplace_node);
		expression_node->ast_type = Node_Type::EXPRESSION;
		expression_node->sibling = nullptr;	// @Warning we have to do this initialization because the expression can be empty

		parse_expression(stream, (AST_Node**)&expression_node->first_child, true, delimiter_1, delimiter_2);
		return;
	}


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
				return;
			}
			else if (current_token.value.punctuation == Punctuation::OPEN_PARENTHESIS)
			{
				// @TODO fix sub-tree now?
			}
			else if (current_token.value.punctuation == Punctuation::STAR) {

			}
			else if (current_token.value.punctuation == Punctuation::SLASH) {

			}
			else if (current_token.value.punctuation == Punctuation::PERCENT) {

			}
			else if (current_token.value.punctuation == Punctuation::PLUS) {

			}
			else if (current_token.value.punctuation == Punctuation::DASH) {

			}
			else if (current_token.value.punctuation == Punctuation::DOT) {
				core::Assert(previous_child != nullptr);
				AST_MEMBER_ACCESS* member_access_node = allocate_AST_node<AST_MEMBER_ACCESS>(emplace_node);

				member_access_node->ast_type = Node_Type::MEMBER_ACCESS;
				member_access_node->sibling = nullptr;
				member_access_node->left = previous_child;
				member_access_node->right = nullptr;

				previous_child = (AST_Node*)member_access_node;
				stream::peek(stream); // .

				parse_expression(stream, &member_access_node->right, true, delimiter_1, delimiter_2);

			}
			// @TODO add other arithmetic operators (bits operations,...)
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
			Token	identifier = current_token;

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

				previous_child = (AST_Node*)identifier_node;
				// Identifier was already peeked
			}
	
			// followed by parenthesis it's a function call
			// followed by brackets it's an array accessor (@TODO take care of multiple arrays)
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
}

void parse_scope(stream::Array_Stream<Token>& stream, AST_Statement_Scope** scope_node_, bool is_root_node /* = false */)
{
	Token					current_token;
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
			if (current_token.value.keyword == Keyword::ALIAS) {
				stream::peek(stream); // alias

				parse_alias(stream, current_child);
				current_child = &(*current_child)->sibling;
			}
			else if (current_token.value.keyword == Keyword::IMPORT) {
				stream::peek<Token>(stream);
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

			Token	identifier = current_token;

			stream::peek<Token>(stream);
			current_token = stream::get(stream);

			if (current_token.type == Token_Type::SYNTAXE_OPERATOR) {
				if (current_token.value.punctuation == Punctuation::DOUBLE_COLON) { // It's a function, struct or enum declaration
					stream::peek<Token>(stream);
					current_token = stream::get(stream);

					if (current_token.type == Token_Type::KEYWORD
						&& current_token.value.keyword == Keyword::STRUCT) {
						stream::peek(stream); // struct

						parse_struct(stream, identifier, current_child);
						current_child = &(*current_child)->sibling;
					}
					else if (current_token.type == Token_Type::KEYWORD
						&& current_token.value.keyword == Keyword::ENUM) {
						stream::peek(stream); // enum

						parse_enum(stream, identifier, current_child);
						current_child = &(*current_child)->sibling;
					}
					else if (current_token.type == Token_Type::SYNTAXE_OPERATOR
						&& current_token.value.punctuation == Punctuation::OPEN_PARENTHESIS) {
						stream::peek(stream); // (

						parse_function(stream, identifier, current_child);
						current_child = &(*current_child)->sibling;
					}
					else {
						report_error(Compiler_Error::error, current_token, "Expecting struct, enum or function parameters list after the '::' token.");
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
				parse_scope(stream, (AST_Statement_Scope**)current_child);
				current_child = &(*current_child)->sibling;	// Move the current_child to the sibling
			}
		}
	}
}

void f::parse(fstd::memory::Array<Token>& tokens, AST& ast)
{
	ZoneScopedNC("f::parse", 0xff6f00);

	stream::Array_Stream<Token>	stream;

	// It is impossible to have more nodes than tokens, so we can easily pre-allocate them to the maximum possible size.
	// maximum_size = nb_tokens * largest_node_size
	//
	// @TODO
	// We expect to be able to determine with the meta-programmation which AST_Node type is the largest one.
	//
	// Flamaros - 13 april 2020
	memory::reserve_array(globals.parser_data.ast_nodes, memory::get_array_size(tokens) * sizeof(AST_Statement_Variable));

	stream::initialize_memory_stream<Token>(stream, tokens);

	if (stream::is_eof(stream) == true) {
		return;
	}

	parse_scope(stream, (AST_Statement_Scope**)&ast.root, true);
}

static void write_dot_node(String_Builder& file_string_builder, AST_Node* node, int64_t parent_index = -1, int64_t left_node_index = -1)
{
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
			"\n%v", magic_enum::enum_name(node->ast_type), alias_node->type_name.text);
	}
	else if (node->ast_type == Node_Type::STATEMENT_BASIC_TYPE) {
		AST_Statement_Basic_Type*	basic_type_node = (AST_Statement_Basic_Type*)node;

		print_to_builder(file_string_builder,
			"%Cv"
			"\n%Cv", magic_enum::enum_name(node->ast_type), magic_enum::enum_name(basic_type_node->keyword));
	}
	else if (node->ast_type == Node_Type::STATEMENT_USER_TYPE) {
		AST_Statement_User_Type*	user_type_node = (AST_Statement_User_Type*)node;

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
			"\nname: %v (is_parameter: %d is_optional: %d)", magic_enum::enum_name(node->ast_type), variable_node->name.text, variable_node->is_function_paramter, variable_node->is_optional);
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
	else if (node->ast_type == Node_Type::EXPRESSION) {
		AST_Expression* expression_node = (AST_Expression*)node;

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
	else if (node->ast_type == Node_Type::MEMBER_ACCESS) {
		AST_MEMBER_ACCESS* member_access_node = (AST_MEMBER_ACCESS*)node;

		print_to_builder(file_string_builder,
			"%Cv", magic_enum::enum_name(node->ast_type));
	}
	else {
		core::Assert(false);
		print_to_builder(file_string_builder,
			"UNKNOWN type"
			"\nnode_%ld", node_index);
	}
	print_to_builder(file_string_builder, "\t" "\" shape=box, style=filled, color=black, fillcolor=lightseagreen]\n");

	// Children iteration
	if (node->ast_type == Node_Type::TYPE_ALIAS) {
		AST_Alias*	alias_node = (AST_Alias*)node;
		write_dot_node(file_string_builder, alias_node->type, node_index);
	}
	else if (node->ast_type == Node_Type::STATEMENT_BASIC_TYPE) {
		// No children
	}
	else if (node->ast_type == Node_Type::STATEMENT_USER_TYPE) {
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
	else if (node->ast_type == Node_Type::EXPRESSION) {
		AST_Expression* expression_node = (AST_Expression*)node;

		write_dot_node(file_string_builder, (AST_Node*)expression_node->first_child, node_index);
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
	else if (node->ast_type == Node_Type::MEMBER_ACCESS) {
		AST_MEMBER_ACCESS* member_access_node = (AST_MEMBER_ACCESS*)node;

		write_dot_node(file_string_builder, (AST_Node*)member_access_node->left, node_index);
		write_dot_node(file_string_builder, (AST_Node*)member_access_node->right, node_index);
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

void f::generate_dot_file(AST& ast, const system::Path& output_file_path)
{
	ZoneScopedNC("f::generate_dot_file", 0xc43e00);

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

	write_dot_node(string_builder, ast.root);

	print_to_builder(string_builder, "}\n");

	file_content = to_string(string_builder);

	system::write_file(file, language::to_utf8(file_content), (uint32_t)language::get_string_size(file_content));
}
