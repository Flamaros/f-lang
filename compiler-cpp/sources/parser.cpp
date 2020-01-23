#include "parser.hpp"

#include "lexer.hpp"

#include <third-party/magic_enum.hpp>

#include <string>
#include <vector>
#include <iostream>
#include <stack>

#include <assert.h>

#undef max
#include <tracy/Tracy.hpp>

// Typedef complexity
// https://en.cppreference.com/w/cpp/language/typedef

namespace f
{
	enum class State
	{
		GLOBAL_SCOPE,
		COMMENT_LINE,
		COMMENT_BLOCK,
		MACRO_EXPRESSION,
        IMPORT_DIRECTIVE,

		eof
	};

	static bool	is_one_line_state(State state)
	{
		return state == State::MACRO_EXPRESSION	// Actually we don't manage every macro directive (we stay on this state)
			|| state == State::COMMENT_LINE;
	}

    void parse(const std::vector<Token>& tokens, AST& ast)
	{
		ZoneScoped;

		std::stack<State>	states;
		Token				name_token;
		size_t				previous_line = 0;
		std::string_view    string_litteral;
		bool				in_string_literal = false;
		bool				start_new_line = true;
		const char*			string_views_buffer = nullptr;	// @Warning all string views are about this string_views_buffer

		// Those stacks should have the same size
		// As stack to handle members and methods

		// @Warning used for debug purpose
		size_t  print_start = 0;
		size_t  print_end = 0;

		states.push(State::GLOBAL_SCOPE);

		if (tokens.size()) {
			string_views_buffer = tokens[0].text.data();	// @Warning all string views are about this string_views_buffer
		}

		for (const Token& token : tokens)
		{
			State& state = states.top();

			if (token.line > previous_line) {
				start_new_line = true;
			}

			// Handle here states that have to be poped on new line detection
			if (start_new_line
				&& is_one_line_state(state))
			{
				states.pop();
				state = states.top();
			}

			if (token.line >= print_start && token.line < print_end) {
				std::cout
					<< std::string(states.size() - 1, ' ') << magic_enum::enum_name(state)
					<< " " << token.line << " " << token.column << " " << token.text << std::endl;
			}

			if (state == State::COMMENT_BLOCK)
			{
				if (token.value.punctuation == Punctuation::CLOSE_BLOCK_COMMENT)
					states.pop();
			}
			else if (state == State::MACRO_EXPRESSION)
			{
                if (token.value.keyword == Keyword::IMPORT)
				{
					states.pop();
                    states.push(State::IMPORT_DIRECTIVE);
				}
			}
            else if (state == State::IMPORT_DIRECTIVE)
			{
				if (token.value.punctuation == Punctuation::DOUBLE_QUOTE
					|| token.value.punctuation == Punctuation::LESS
					|| token.value.punctuation == Punctuation::GREATER)
				{
/*					if (in_string_literal)
					{
						Include	include;

						include.type = (token.value.punctuation == Punctuation::greater) ? Include_Type::external : Include_Type::local;
						include.path = string_litteral;
						result.includes.push_back(include);

						in_string_literal = false;
						states.pop();
					}
					else
					{
						in_string_literal = true;
						string_litteral = std::string_view();
                    }*/
				}
				else if (token.value.keyword == Keyword::UNKNOWN
					&& token.value.punctuation == Punctuation::UNKNOWN)
				{
					// Building the string litteral (can be splitted into multiple tokens)
					if (in_string_literal)
					{
						if (string_litteral.length() == 0)
							string_litteral = token.text;
						else
							string_litteral = std::string_view(
								string_litteral.data(),
								(token.text.data() + token.text.length()) - string_litteral.data());    // Can't simply add length of string_litteral and token.text because white space tokens are skipped
					}
				}
			}
			else if (state == State::GLOBAL_SCOPE)
			{
				if (start_new_line	// @Warning to be sure that we are on the beginning of the line
					&& token.value.punctuation == Punctuation::HASH) {   // Macro
					states.push(State::MACRO_EXPRESSION);
				}
				else if (token.value.punctuation == Punctuation::OPEN_BLOCK_COMMENT) {
					states.push(State::COMMENT_BLOCK);
				}
				else if (token.value.punctuation == Punctuation::LINE_COMMENT) {
					states.push(State::COMMENT_LINE);
				}
			}
			start_new_line = false;
			previous_line = token.line;
		}

		// @Warning we should finish on the global_scope state or one that can stay active only on one line
		assert(states.size() >= 1 && states.size() <= 2);
		assert(states.top() == State::GLOBAL_SCOPE
			|| is_one_line_state(states.top()));
	}
}
