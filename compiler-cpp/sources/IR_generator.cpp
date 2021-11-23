#include "IR_generator.hpp"

#include "parser/parser.hpp"

using namespace fstd;
using namespace fstd::core;

using namespace f;

void f::generate_ir(Parsing_Result& ast, IR& ir)
{
	ir.ast = &ast;
}
