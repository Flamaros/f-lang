#pragma once

#include "f_tokenizer.hpp"

#include <vector>
#include <string_view>

namespace f
{
	enum class Include_Type
	{
		local,
		external
	};

	struct Include
	{
		Include_Type		type;
		std::string_view	path;
	};

	struct Parsing_Result
	{
		std::vector<Include>	includes;
	};

	void parse(const std::vector<Token>& tokens, Parsing_Result& result);
}
