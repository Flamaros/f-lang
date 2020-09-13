#pragma once

#include "parser/parser.hpp"

#include <fstd/system/path.hpp>

namespace f
{
	struct IR
	{
		AST*	ast;
	};

	void generate_ir(AST& ast, IR& ir);
}
