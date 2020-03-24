#pragma once

#include "../lexer/lexer.hpp"

#include <fstd/memory/array.hpp>

#include <fstd/system/path.hpp>

namespace f
{
	enum class Expression_Type
	{
		NUMBER,
		IDENTIFIER,				/// Variable identifier
		BINARY_OPERATION,		/// Operation that takes 2 arguments (+ - * / %)



		MODULE,					/// A source file is an implicit module
		IMPORT,
		ASSIGNMENT,
		FUNCTION_CALL,
		FUNCTION_DEFINITION,
		CONDITION,
	};

	enum class Node_Type
	{
		TYPE_ALIAS,
		TYPE_ENUM,
		TYPE_STRUCT,

		DECLARATION_VARIABLE,
		DECLARATION_FUNCTION,

		ASSIGNMENT,
		BINARY_OP,
		UNARY_OP,
		FUNCTION_CALL
	};

	struct AST_Node
	{
		Node_Type	type;
		uint8_t		reserved[8];
	};

	struct Type_Node
	{
		// @TODO in f-lang we should be able to use C style inheritance
		// https://youtu.be/ZHqFrNyLlpA?t=1905
		// uncomment the following line:
		// using node : AST_Node;	// node is both named and anonymous

		// @TODO
		// In f-lang the AST_Node should not reserve some memory, but instead
		// we should be able to look at all types that encapsulate AST_Node at compile
		// time and determinate the size of the largest struct.
	};

    struct AST
	{
		AST_Node*	root;
	};

	struct Parser_Data
	{
		fstd::memory::Array<f::AST_Node>	ast_nodes;
	};

    void parse(fstd::memory::Array<Token>& tokens, AST& ast);
	void generate_dot_file(AST& ast, const fstd::system::Path& output_file_path);
}
