#pragma once

#include "../lexer/lexer.hpp"

#include <fstd/memory/array.hpp>

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

	typedef uint32_t AST_Node_Index;

	struct AST_Node
	{
		Expression_Type	type;
		Token			token;
		AST_Node*		first_sibling;
		AST_Node*		first_child;
	};

    struct AST
	{
		AST_Node*	root;
	};

    void parse(const fstd::memory::Array<Token>& tokens, AST& ast);
}
