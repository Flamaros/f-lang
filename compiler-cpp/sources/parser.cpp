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

	void parse(const fstd::memory::Array<Token>& tokens, AST& ast)
	{
		ZoneScoped;
	}
}
