#include "IR_generator.hpp"

#include "parser/parser.hpp"

using namespace fstd;
using namespace fstd::core;

using namespace f;

void f::generate_irl(AST& ast, IR& ir)
{
	ir.ast = &ast;
}
