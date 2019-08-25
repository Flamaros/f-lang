#include "../f_tokenizer.hpp"
#include "../f_parser.hpp"

#include <CppUnitTest.h>

#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace f;

namespace tests
{
	TEST_CLASS(macro_tokenizer_tests)
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
	};
}
