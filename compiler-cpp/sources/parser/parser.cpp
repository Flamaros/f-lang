#include "parser.hpp"

#include "globals.hpp"

#include <fstd/memory/array.hpp>

#include <fstd/stream/array_stream.hpp>

#undef max
#include <tracy/Tracy.hpp>

// Typedef complexity
// https://en.cppreference.com/w/cpp/language/typedef

using namespace fstd;

enum class State
{
	GLOBAL_SCOPE,
	COMMENT_LINE,
	COMMENT_BLOCK,
	IMPORT_DIRECTIVE,

	eof
};

inline f::AST_Node* allocate_AST_node()
{
	// Ensure that no reallocation could happen during the resize
	core::Assert(memory::get_array_size(globals.parser_data.ast_nodes) < memory::get_array_reserved(globals.parser_data.ast_nodes));

	memory::resize_array(globals.parser_data.ast_nodes, memory::get_array_size(globals.parser_data.ast_nodes) + 1);
	return memory::get_array_last_element(globals.parser_data.ast_nodes);
}

inline void parse_type_qualifier(stream::Array_Stream<f::Token>& stream, f::Type_Qualifier* type_qualifier)
{
	type_qualifier->name = stream::get(stream);
	// TODO parse until a token that can't be interpreted
}

void f::parse(fstd::memory::Array<Token>& tokens, AST& ast)
{
	ZoneScopedNC("f::parse", 0xff6f00);

	stream::Array_Stream<Token>	stream;

	// It is impossible to have more nodes than tokens, so we can easily pre-allocate them.
	//
	// Flamaros - 23 march 2020
	memory::reserve_array(globals.parser_data.ast_nodes, memory::get_array_size(tokens));

	stream::initialize_memory_stream<Token>(stream, tokens);

	if (stream::is_eof(stream) == true) {
		return;
	}

	while (stream::is_eof(stream) == false)
	{
		Token	current_token;

		current_token = stream::get(stream);

		if (current_token.type == Token_Type::KEYWORD) {
			if (current_token.value.keyword == Keyword::ALIAS) {
				stream::peek(stream);

				Type_Alias_Node* new_node = (Type_Alias_Node*)allocate_AST_node();

				new_node->type = Node_Type::TYPE_ALIAS;
				new_node->type_name = stream::get(stream);
				stream::peek(stream);
				current_token = stream::get(stream);
				if (current_token.type != Token_Type::SYNTAXE_OPERATOR
					&& current_token.value.punctuation != Punctuation::EQUALS) {
					report_error(Compiler_Error::error, current_token, "Expecting '=' punctuation after the alias typename.");
				}
				stream::peek(stream);
				parse_type_qualifier(stream, &new_node->qualifier);

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

			stream::peek<Token>(stream);
		}

	}
}

void f::generate_dot_file(AST& ast, const system::Path& output_file_path)
{
	ZoneScopedNC("f::generate_dot_file", 0xc43e00s);

}
