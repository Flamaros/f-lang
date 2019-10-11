#pragma once

#include "lexer.hpp"

#include <vector>
#include <string_view>

namespace f
{
	enum class Expression_Type
	{
		number,
		identifier,				/// Variable identifier
		binary_operation,		/// Operation that takes 2 arguments (+ - * / %)



		module,					/// A source file is an implicit module
		import,
		assignment,
		function_call,
		function_definition,
		condition,
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

    void parse(const std::vector<Token>& tokens, AST& ast);
}
