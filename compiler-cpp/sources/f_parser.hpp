#pragma once

#include "f_tokenizer.hpp"

#include <vector>
#include <string_view>

namespace f
{
    struct Type
    {
        enum class Kind
        {
            _native,    // i32, ui32,...
            _struct,
            _enum,
            _function,  // function ptr
        };

        Kind                type;
        std::vector<Token>  full_qualifier;
    };

    struct Variable
    {
        f::Token    name;
        Type        type;
    };

    struct Function
    {
        f::Token                name;
        Type                    return_type;
        std::vector<Variable>   arguments;
    };

    struct Parsing_Result
	{
        std::vector<Function>	functions;
        std::vector<Variable>	variables;
        std::vector<Type>       types;
	};

	void parse(const std::vector<Token>& tokens, Parsing_Result& result);
}
