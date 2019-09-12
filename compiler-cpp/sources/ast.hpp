#pragma once

#include "lexer.hpp"

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

    struct AST
	{
        std::vector<Function>	functions;
        std::vector<Variable>	variables;
        std::vector<Type>       types;
	};

    void build_ast(const std::vector<Token>& tokens, AST& result);
}
