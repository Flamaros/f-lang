#include "../lexer.hpp"

#include <CppUnitTest.h>

#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace f;

namespace tests
{
	TEST_CLASS(lexer_tests)
	{
	public:
		
		TEST_METHOD(empty_text)
		{
			std::vector<Token>	tokens;
			std::string			text =
				"";

			tokenize(text, tokens);

			Assert::AreEqual(tokens.size(), size_t(0));
		}

		TEST_METHOD(white_spaces_text)
		{
			std::vector<Token>	tokens;
			std::string			text =
				"  \t  \r\n";

			tokenize(text, tokens);

			Assert::AreEqual(tokens.size(), size_t(0));
		}

		TEST_METHOD(comment_line_text_01)
		{
			std::vector<Token>	tokens;
			std::string			text =
				"// Test";

			tokenize(text, tokens);

			Assert::AreEqual(tokens.size(), size_t(0));
		}

		TEST_METHOD(comment_line_text_02)
		{
			std::vector<Token>	tokens;
			std::string			text =
				"/// Test";

			tokenize(text, tokens);

			Assert::AreEqual(tokens.size(), size_t(0));
		}

		TEST_METHOD(comment_line_text_03)
		{
			std::vector<Token>	tokens;
			std::string			text =
				"/// Test\r\n"
				"/// Test";

			tokenize(text, tokens);

			Assert::AreEqual(tokens.size(), size_t(0));
		}

		TEST_METHOD(comment_block_text_01)
		{
			std::vector<Token>	tokens;
			std::string			text =
				"/* Test\r\n"
				"   Test\n"
				"   Test *//* Test\r\n"
				"   Test\n"
				"   Test */";

			tokenize(text, tokens);

			Assert::AreEqual(tokens.size(), size_t(0));
		}

		TEST_METHOD(numeric_literals_integer)
		{
			std::vector<Token>	tokens;
			std::string			text =
				"0\n"				// i32
				"10\n"				// i32
				"-10\n"				// i32 @Warning 2 tokens here: '-' and '10', the parser will handle the minus sign as unary operator
				"2_147_483_647\n"	// i32
				"2_147_483_648\n"	// i64
				"256L\n"			// i64
				"4_294_967_295u\n"	// ui32
				"4_294_967_296u\n"	// ui64
				"256uL\n"			// ui64
				"256Lu\n"			// ui64
				"0b1001\n"			// i32 binary
				"0xFFBBAAddee"		// i64 hexadecimal (>= 2_147_483_648)
				;

			tokenize(text, tokens);

			Assert::AreEqual(size_t(13), tokens.size());
			Assert::AreEqual((int)Token_Type::numeric_literal_i32, (int)tokens[0].type);
			Assert::AreEqual(0, (int32_t)tokens[0].value.integer);
			Assert::AreEqual((int)Token_Type::numeric_literal_i32, (int)tokens[1].type);
			Assert::AreEqual(10, (int32_t)tokens[1].value.integer);
			Assert::AreEqual((int)Token_Type::numeric_literal_i32, (int)tokens[3].type);
			Assert::AreEqual(10, (int32_t)tokens[3].value.integer);
			Assert::AreEqual((int)Token_Type::numeric_literal_i32, (int)tokens[4].type);
			Assert::AreEqual(2'147'483'647, (int32_t)tokens[4].value.integer);
			Assert::AreEqual((int)Token_Type::numeric_literal_i64, (int)tokens[5].type);
			Assert::AreEqual(2'147'483'648LL, (int64_t)tokens[5].value.integer);
			Assert::AreEqual((int)Token_Type::numeric_literal_i64, (int)tokens[6].type);
			Assert::AreEqual(256LL, (int64_t)tokens[6].value.integer);
			Assert::AreEqual((int)Token_Type::numeric_literal_ui32, (int)tokens[7].type);
			Assert::AreEqual(4'294'967'295u, (uint32_t)tokens[7].value.integer);
			Assert::AreEqual((int)Token_Type::numeric_literal_ui64, (int)tokens[8].type);
			Assert::AreEqual(429'496'7296u, (uint64_t)tokens[8].value.integer);
			Assert::AreEqual((int)Token_Type::numeric_literal_ui64, (int)tokens[9].type);
			Assert::AreEqual(256uLL, (uint64_t)tokens[9].value.integer);
			Assert::AreEqual((int)Token_Type::numeric_literal_ui64, (int)tokens[10].type);
			Assert::AreEqual(256uLL, (uint64_t)tokens[10].value.integer);
			Assert::AreEqual((int)Token_Type::numeric_literal_i32, (int)tokens[11].type);
			Assert::AreEqual(0b1001, (int32_t)tokens[11].value.integer);
			Assert::AreEqual((int)Token_Type::numeric_literal_i64, (int)tokens[12].type);
			Assert::AreEqual(0xFFBBAAddee, (int64_t)tokens[12].value.integer);
		}

		TEST_METHOD(numeric_literals_real)
		{
			std::vector<Token>	tokens;
			std::string			text =
				"2.645_751\n"
				"6.022140857e+23\n"
				"6_022.140857e+20\n"
				"6_022_.140_857e+20_\n"
				"1.175494351e-38f\n"			// float.min
				"0x1.FFFFFFFFFFFFFp1023\n"		// double.max
				//"0x1p-52\n"					// double.epsilon
				;

			tokenize(text, tokens);

			Assert::AreEqual(size_t(6), tokens.size());
			Assert::AreEqual((int)Token_Type::numeric_literal_f64, (int)tokens[0].type);
			Assert::AreEqual(2.645'751, tokens[0].value.real_64);
			Assert::AreEqual((int)Token_Type::numeric_literal_f64, (int)tokens[1].type);
			Assert::AreEqual(6.022140857e+23, tokens[1].value.real_64, 0.000000001 * pow(10, 23));
			Assert::AreEqual((int)Token_Type::numeric_literal_f64, (int)tokens[2].type);
			Assert::AreEqual(6'022.140'857e+20, tokens[2].value.real_64, 0.000000001 * pow(10, 23));
			Assert::AreEqual((int)Token_Type::numeric_literal_f64, (int)tokens[3].type);
			Assert::AreEqual(6'022.140'857e+20, tokens[3].value.real_64, 0.000000001 * pow(10, 23));
			Assert::AreEqual((int)Token_Type::numeric_literal_f32, (int)tokens[4].type);
			Assert::AreEqual(1.175494351e-38f, tokens[4].value.real_32, std::numeric_limits<float>::epsilon());
			Assert::AreEqual((int)Token_Type::numeric_literal_f64, (int)tokens[5].type);
			Assert::AreEqual(std::numeric_limits<double>::max(), tokens[5].value.real_64);
		}

		TEST_METHOD(string_literals)
		{
			std::vector<Token>	tokens;
			std::string			text = "\"Hello World\"";

			// TODO test escape characters

			tokenize(text, tokens);
			Assert::AreEqual(size_t(1), tokens.size());
			Assert::AreEqual(std::string("Hello World"), std::string(tokens[0].text));
		}

		//TEST_METHOD(special_kewords)
		//{
		//	std::vector<Token>	tokens;
		//	std::string			text = "";

		//	tokenize(text, tokens);
		//	Assert::AreEqual(size_t(-1), tokens.size());
		//}

		TEST_METHOD(simple_code)
		{
			std::vector<Token>	tokens;
			std::string			text =
				"import f_language_definitions;\n"										// 3
				"\n"
				"typedef DWORD = ui32;\n"												// 8
				"typedef HANDLE = void*;\n"												// 14
				"\n"
				"main :: (arguments : [] string) -> i32\n"								// 25
				"{\n"																	// 26
				"message:        string = \"Hello World\";\n"							// 32
				"written_bytes:  DWORD = 0;\n"											// 38
				"hstdOut:        void* = GetstdHandle(STD_OUTPUT_HANDLE);\n"			// 48
				"\n"
				"	WriteFile(hstdOut, message.c_string, message.length, &bytes, 0);\n"	// 66
				"\n"
				"	// ExitProcess(0);\n"												// 66
				"	return 0;\n"														// 69
				"}"																	// 70
			;

			tokenize(text, tokens);
			Assert::AreEqual(size_t(70), tokens.size());
			// TODO need to fix string litteral and comments
		}
	};
}
