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

enum class State
{
	GLOBAL_SCOPE,
	COMMENT_LINE,
	COMMENT_BLOCK,
	IMPORT_DIRECTIVE,

	eof
};

template<typename Node_Type>
inline Node_Type* allocate_AST_node(f::AST_Node** previous_sibling_addr)
{
	// @TODO f-lang
	// Check if Node_Type is an AST_Node at compile time
	//
	// Flamaros - 13 april 2020

	// Ensure that no reallocation could happen during the resize
	core::Assert(memory::get_array_size(globals.parser_data.ast_nodes) < memory::get_array_reserved(globals.parser_data.ast_nodes) * sizeof(f::AST_Statement_Variable));

	memory::resize_array(globals.parser_data.ast_nodes, memory::get_array_size(globals.parser_data.ast_nodes) + sizeof(Node_Type));

	Node_Type* new_node = (Node_Type*)(memory::get_array_last_element(globals.parser_data.ast_nodes) - sizeof(Node_Type));
	if (previous_sibling_addr) {
		*previous_sibling_addr = (f::AST_Node*)new_node;
	}
	return new_node;
}

inline void parse_array(stream::Array_Stream<f::Token>& stream, f::AST_Node** type_)
{
	f::Token	current_token;

	stream::peek(stream); // [
// @TODO Implement it
	while (true)
	{
		current_token = stream::get(stream);
		if (current_token.value.punctuation == f::Punctuation::CLOSE_BRACKET) {
			stream::peek(stream);
			return;
		}
		stream::peek(stream);
	}
}

inline void parse_type(stream::Array_Stream<f::Token>& stream, f::AST_Node** type_node)
{
	f::Token		current_token;
	f::AST_Node**	previous_sibling_addr = nullptr;

	// A type sequence will necessary ends with the type name (basic type keyword or identifier), all modifiers comes before.
	//
	// Flamaros - 01 may 2020

	while (true)
	{
		current_token = stream::get(stream);

		if (current_token.type == f::Token_Type::SYNTAXE_OPERATOR) {
			if (current_token.value.punctuation == f::Punctuation::STAR) {
				f::AST_Statement_Type_Pointer*	pointer_node = allocate_AST_node<f::AST_Statement_Type_Pointer>(previous_sibling_addr);
				previous_sibling_addr = (f::AST_Node**)&pointer_node->sibling;

				if (*type_node == nullptr) {
					*type_node = (f::AST_Node*)pointer_node;
				}

				pointer_node->ast_type = f::Node_Type::STATEMENT_TYPE_POINTER;
				pointer_node->sibling = nullptr;

				stream::peek(stream); // *
			}
			else if (current_token.value.punctuation == f::Punctuation::OPEN_BRACKET) {
				parse_array(stream, previous_sibling_addr);
			}
		}
		else if (current_token.type == f::Token_Type::KEYWORD) {
			if (f::is_a_basic_type(current_token.value.keyword)) {
				f::AST_Statement_Basic_Type*	basic_type_node = allocate_AST_node<f::AST_Statement_Basic_Type>(previous_sibling_addr);

				if (*type_node == nullptr) {
					*type_node = (f::AST_Node*)basic_type_node;
				}

				basic_type_node->ast_type = f::Node_Type::STATEMENT_BASIC_TYPE;
				basic_type_node->sibling = nullptr;
				basic_type_node->keyword = current_token.value.keyword;

				stream::peek(stream); // basic_type keyword
				break;
			}
			else {
				report_error(Compiler_Error::error, current_token, "Expecting a type qualifier (a type modifier operator, a basic type keyword or a user type identifier).");
			}
		}
		else if (current_token.type == f::Token_Type::IDENTIFIER) {
			// @TODO Implement it
			break;
		}
		else {
			report_error(Compiler_Error::error, current_token, "Expecting a type qualifier (a type modifier operator, a basic type keyword or a user type identifier).");
		}
	}
}

inline void parse_function_argument(stream::Array_Stream<f::Token>& stream, f::AST_Statement_Variable** parameter_)
{
	f::Token					current_token;
	f::AST_Statement_Variable*& parameter = *parameter_;

	current_token = stream::get(stream);
	fstd::core::Assert(current_token.type == f::Token_Type::IDENTIFIER);

	parameter = allocate_AST_node<f::AST_Statement_Variable>(nullptr);
	parameter->ast_type = f::Node_Type::STATEMENT_VARIABLE;
	parameter->sibling = nullptr;
	parameter->name = current_token;
	parameter->is_function_paramter = true;

	stream::peek(stream); // identifier

	current_token = stream::get(stream);
	if (current_token.type == f::Token_Type::SYNTAXE_OPERATOR
		&& current_token.value.punctuation == f::Punctuation::COLON) {
		stream::peek(stream); // :

		parse_type(stream, &parameter->type);
		current_token = stream::get(stream);

		if (current_token.type == f::Token_Type::SYNTAXE_OPERATOR
			&& current_token.value.punctuation == f::Punctuation::EQUALS) {
			parameter->is_optional = true;
			parameter->expression;
			// @TODO
			fstd::core::Assert(false);
		}
		else {
			parameter->is_optional = false;
			parameter->expression = nullptr;
		}
	}
	else {
		report_error(Compiler_Error::error, current_token, "Expecting ':' between the type and the name of the paramater of function.");
	}
}

inline void parse_scope(stream::Array_Stream<f::Token>& stream, f::AST_Node** type_)
{
	f::Token	current_token;

	stream::peek(stream); // [
// @TODO Implement it
	while (true)
	{
		current_token = stream::get(stream);
		if (current_token.value.punctuation == f::Punctuation::CLOSE_BRACE) {
			stream::peek(stream);
			return;
		}
		stream::peek(stream);
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

	AST_Node** previous_sibling_addr = &ast.root;

	while (stream::is_eof(stream) == false)
	{
		Token	current_token;

		current_token = stream::get(stream);

		if (current_token.type == Token_Type::KEYWORD) {
			if (current_token.value.keyword == Keyword::ALIAS) {
				stream::peek(stream); // alias

				AST_Alias*	alias_node = allocate_AST_node<AST_Alias>(previous_sibling_addr);
				previous_sibling_addr = (AST_Node**)&alias_node->sibling;

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
			else if (current_token.value.keyword == Keyword::IMPORT) {
				stream::peek<Token>(stream);

			}
			else
			{
				report_error(Compiler_Error::error, current_token, "Unexpected keyword in the current context (global scope).");
			}
		}
		else if (current_token.type == Token_Type::IDENTIFIER) {
			// At global scope we can only have variable or function declarations that start with an identifier

			Token	identifier = current_token;

			stream::peek<Token>(stream);
			current_token = stream::get(stream);

			if (current_token.type == Token_Type::SYNTAXE_OPERATOR) {
				if (current_token.value.punctuation == Punctuation::DOUBLE_COLON) { // It's a function, struct or enum declaration
					stream::peek<Token>(stream);
					current_token = stream::get(stream);

					if (current_token.type == Token_Type::KEYWORD
						&& current_token.value.keyword == Keyword::STRUCT) {
					}
					else if (current_token.type == Token_Type::KEYWORD
						&& current_token.value.keyword == Keyword::ENUM) {
					}
					else if (current_token.type == Token_Type::SYNTAXE_OPERATOR
						&& current_token.value.punctuation == Punctuation::OPEN_PARENTHESIS) {
						stream::peek(stream); // (

						AST_Statement_Function* function_node = allocate_AST_node<AST_Statement_Function>(previous_sibling_addr);
						previous_sibling_addr = (AST_Node**)&function_node->sibling;

						function_node->ast_type = Node_Type::STATEMENT_FUNCTION;
						function_node->sibling = nullptr;
						function_node->name = identifier;
						function_node->nb_arguments = 0;
						function_node->arguments = nullptr;

						current_token = stream::get(stream);

						AST_Statement_Variable** current_arguments = &function_node->arguments;
						while (!(current_token.type == Token_Type::SYNTAXE_OPERATOR
							&& current_token.value.punctuation == Punctuation::CLOSE_PARENTHESIS))
						{
							if (current_token.type == Token_Type::IDENTIFIER)
							{
								parse_function_argument(stream, current_arguments);
								current_token = stream::get(stream);

								current_arguments = (AST_Statement_Variable**)&(*current_arguments)->sibling;
								if (current_token.type == Token_Type::SYNTAXE_OPERATOR &&
									current_token.value.punctuation == Punctuation::COMMA)
								{
									stream::peek(stream); // ,
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
							parse_scope(stream, nullptr);
							current_token = stream::get(stream);
						}
						else {
							report_error(Compiler_Error::error, current_token, "Expecting '->' to specify the return type of the function or the scope of the function.");
						}
					}
					else {
						report_error(Compiler_Error::error, current_token, "Expecting struct, enum or function parameters list after the '::' token.");
					}
				}
				else if (current_token.value.punctuation != Punctuation::COLON) { // It's a variable declaration with type

				}
				else if (current_token.value.punctuation != Punctuation::COLON_EQUAL) { // It's a variable declaration where type is infered

				}
				else if (current_token.value.punctuation != Punctuation::EQUALS) { // It's a variable assignement

				}
			}
		}
	}
}

void write_dot_node(String_Builder& file_string_builder, f::AST_Node* node, int64_t parent_index = -1)
{
	language::string	dot_node;
	static uint32_t		nb_nodes = 0;
	uint32_t			node_index = nb_nodes++;
	f::AST_Node*		next_node = nullptr;

	defer{
		release(dot_node);
	};

	if (parent_index != -1) {
		print_to_builder(file_string_builder, "\t" "node_%ld -> node_%ld\n", parent_index, node_index);
	}

	print_to_builder(file_string_builder, "\n\t" "node_%ld [label=\"", node_index);
	if (node->ast_type == f::Node_Type::TYPE_ALIAS) {
		f::AST_Alias* alias_node = (f::AST_Alias*)node;

		print_to_builder(file_string_builder,
			"%Cv"
			"\n%v", magic_enum::enum_name(node->ast_type), alias_node->type_name.text);
	}
	else if (node->ast_type == f::Node_Type::STATEMENT_BASIC_TYPE) {
		f::AST_Statement_Basic_Type*	basic_type_node = (f::AST_Statement_Basic_Type*)node;

		print_to_builder(file_string_builder,
			"%Cv"
			"\n%Cv", magic_enum::enum_name(node->ast_type), magic_enum::enum_name(basic_type_node->keyword));
	}
	else if (node->ast_type == f::Node_Type::STATEMENT_TYPE_POINTER) {
		f::AST_Statement_Type_Pointer*	basic_type_node = (f::AST_Statement_Type_Pointer*)node;

		print_to_builder(file_string_builder,
			"%Cv", magic_enum::enum_name(node->ast_type));
	}
	else if (node->ast_type == f::Node_Type::STATEMENT_FUNCTION) {
		f::AST_Statement_Function*	function_node = (f::AST_Statement_Function*)node;

		print_to_builder(file_string_builder,
			"%Cv"
			"\nname: %v (%d)", magic_enum::enum_name(node->ast_type), function_node->name.text, function_node->nb_arguments);
	}
	else {
		core::Assert(false);
		print_to_builder(file_string_builder,
			"UNKNOWN type"
			"\nnode_%ld", node_index);
	}
	print_to_builder(file_string_builder, "\t" "\" shape=box, style=filled, color=black, fillcolor=lightseagreen]\n");

	// Children iteration
	if (node->ast_type == f::Node_Type::TYPE_ALIAS) {
		f::AST_Alias*	alias_node = (f::AST_Alias*)node;
		write_dot_node(file_string_builder, alias_node->type, node_index);
	}
	else if (node->ast_type == f::Node_Type::STATEMENT_BASIC_TYPE) {
		// No children
	}
	else if (node->ast_type == f::Node_Type::STATEMENT_TYPE_POINTER) {
		// No children
	}
	else if (node->ast_type == f::Node_Type::STATEMENT_FUNCTION) {
		// @TODO
	}
	else {
		core::Assert(false);
	}

	// Sibling iteration
	next_node = node->sibling;
	while (next_node) {
		write_dot_node(file_string_builder, next_node, parent_index);
		next_node = next_node->sibling;
	}

	dot_node = to_string(file_string_builder);
}

void f::generate_dot_file(AST& ast, const system::Path& output_file_path)
{
	ZoneScopedNC("f::generate_dot_file", 0xc43e00s);

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
