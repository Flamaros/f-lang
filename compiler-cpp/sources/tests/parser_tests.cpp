#include "../parser.hpp"

#include <CppUnitTest.h>

#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace f;

namespace tests
{
	TEST_CLASS(parser_tests)
	{
	public:
		TEST_METHOD(hello_world)
		{
			std::vector<Token>	tokens;
			AST					ast;
			std::string			text =
				"main :: () -> i32\n"
				"{\n"
				"	printf(\"Hello World\n\");\n"
				"	return 0;\n"
				"}\n";

			tokenize(text, tokens);
			parse(tokens, ast);
		}

		TEST_METHOD(import_and_typedef)
		{
			std::vector<Token>	tokens;
			AST					ast;
			std::string			text =
				"import f_language_definitions;\n"
				"\n"
				"typedef DWORD = ui32;\n"
				"typedef HANDLE = void*;\n"
				"\n"
				"fn main :: (arguments : [] string) -> i32\n"
				"{\n"
				"    message:        string  = \"Hello World\";\n"
				"    written_bytes:  DWORD   = 0;\n"
				"    hstdOut:        void*   = GetstdHandle(STD_OUTPUT_HANDLE);\n"
				"\n"
				"    WriteFile(hstdOut, message.c_string, message.length, &bytes, 0);\n"
				"\n"
				"    // ExitProcess(0);\n"
				"    return 0;\n"
				"}\n";

			tokenize(text, tokens);
			parse(tokens, ast);
		}
	};
}
