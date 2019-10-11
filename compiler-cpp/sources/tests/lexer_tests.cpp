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
				"// ";

			tokenize(text, tokens);

			Assert::AreEqual(tokens.size(), size_t(1));
			Assert::AreEqual((int)tokens[0].punctuation, (int)Punctuation::line_comment);
		}

		TEST_METHOD(comment_line_text_02)
		{
			std::vector<Token>	tokens;
			std::string			text =
				"/// ";

			tokenize(text, tokens);

			Assert::AreEqual(tokens.size(), size_t(2));
			Assert::AreEqual((int)tokens[0].punctuation, (int)Punctuation::line_comment);
		}

		TEST_METHOD(comment_line_text_03)
		{
			std::vector<Token>	tokens;
			std::string			text =
				"///\r\n"
				"///\r\n";

			tokenize(text, tokens);

			Assert::AreEqual(tokens.size(), size_t(4));
			Assert::AreEqual((int)tokens[0].punctuation, (int)Punctuation::line_comment);
			Assert::AreEqual((int)tokens[0].line, (int)1);
			Assert::AreEqual((int)tokens[1].punctuation, (int)Punctuation::slash);
			Assert::AreEqual((int)tokens[1].line, (int)1);
			Assert::AreEqual((int)tokens[2].punctuation, (int)Punctuation::line_comment);
			Assert::AreEqual((int)tokens[2].line, (int)2);
			Assert::AreEqual((int)tokens[3].punctuation, (int)Punctuation::slash);
			Assert::AreEqual((int)tokens[3].line, (int)2);
		}

		TEST_METHOD(numeric_literals)
		{
			std::vector<Token>	tokens;
			std::string			text =	// In favor of lower case letter, but for the Long suffix (i64) we use L suffix as 'l' looks so similar to '1' (one) in lot of fonts
				"0\n"				// i32
				"10\n"				// i32
				"2_147_483_647\n"	// i32
				"2_147_483_648\n"	// i64
				"256L\n"			// i64
				"4_294_967_295u\n"	// ui32
				"4_294_967_296u\n"	// ui64
				"256uL\n"			// ui64
				"256Lu\n"			// ui64
				"0b1001\n"			// i32 binary
				"0xFFBBAAddee\n"	// i64 hexadecimal (>= 2_147_483_648)

				;

			tokenize(text, tokens);

			Assert::AreEqual(tokens.size(), size_t(11));
			Assert::AreEqual((int)Token_Type::numeric_literal_i32, (int)tokens[0].type);
			Assert::AreEqual(0, (int32_t)tokens[0].integer);
			Assert::AreEqual((int)Token_Type::numeric_literal_i32, (int)tokens[1].type);
			Assert::AreEqual(10, (int32_t)tokens[1].integer);
			Assert::AreEqual((int)Token_Type::numeric_literal_i32, (int)tokens[2].type);
			Assert::AreEqual(2'147'483'647, (int32_t)tokens[2].integer);
			Assert::AreEqual((int)Token_Type::numeric_literal_i64, (int)tokens[3].type);
			Assert::AreEqual(2'147'483'648LL, (int64_t)tokens[3].integer);
			Assert::AreEqual((int)Token_Type::numeric_literal_i64, (int)tokens[4].type);
			Assert::AreEqual(256LL, (int64_t)tokens[4].integer);
			Assert::AreEqual((int)Token_Type::numeric_literal_ui32, (int)tokens[5].type);
			Assert::AreEqual(4'294'967'295u, (uint32_t)tokens[5].integer);
			Assert::AreEqual((int)Token_Type::numeric_literal_ui64, (int)tokens[6].type);
			Assert::AreEqual(429'496'7296u, (uint64_t)tokens[6].integer);
			Assert::AreEqual((int)Token_Type::numeric_literal_ui64, (int)tokens[7].type);
			Assert::AreEqual(256uLL, (uint64_t)tokens[7].integer);
			Assert::AreEqual((int)Token_Type::numeric_literal_ui64, (int)tokens[8].type);
			Assert::AreEqual(256uLL, (uint64_t)tokens[8].integer);
			Assert::AreEqual((int)Token_Type::numeric_literal_i32, (int)tokens[9].type);
			Assert::AreEqual(0b1001, (int32_t)tokens[9].integer);
			Assert::AreEqual((int)Token_Type::numeric_literal_i64, (int)tokens[10].type);
			Assert::AreEqual(0xFFBBAAddee, (int64_t)tokens[10].integer);
		}
	};
}
