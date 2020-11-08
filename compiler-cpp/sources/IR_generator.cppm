export module f.ir_generator;

import f.parser;

import fstd.system.path;

namespace f
{
	export
	struct IR
	{
		AST*	ast;
	};

	export void generate_ir(AST& ast, IR& ir);
}

module: private;

import f.parser;

using namespace fstd;
using namespace fstd::core;

using namespace f;

void f::generate_ir(AST& ast, IR& ir)
{
	ir.ast = &ast;
}
