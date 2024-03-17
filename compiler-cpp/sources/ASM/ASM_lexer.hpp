#pragma once

#include "ASM_x64.h"

#include <fstd/memory/array.hpp>

#include <fstd/core/assert.hpp>

#include <fstd/system/path.hpp>

/*
Notice:
 - Inspired by Dlang lexer (https://dlang.org/spec/lex.html#IntegerLiteral), flang lexer don't support
   long double (80 bits) type and imaginary numbers. An other difference is that for prefixes and suffixes
   flang lexer only support lower case in exception of L that looks too closely to 1 in lower case.
 - '-' character is an operator, so it can't be used to declare identifier (variables, functions,...).
   Also numeric_literal never contains negative number, this is the role of the parser do desembiguate
   if this character is used as unary or binary operator and eventually to modify the token value.
*/

namespace f::ASM
{
    enum class Keyword : uint8_t
    {
        UNKNOWN,

        IMPORTS,
        MODULE,
        SECTION,

        // Pseudo instructions (part of the language)
        DB,

		COUNT,
    };

    enum class Token_Type : uint8_t
    {
        // @Warning don't change the order of types
        // Or take care to update functions like is_literal

        UNKNOWN,
        IDENTIFIER,	/// Every word that isn't a part of the language
        KEYWORD,
        INSTRUCTION,    // Not a real keyword because it may depends of the targeted architecture (even if I hard code the support of only x64 arch)
        REGISTER,       // Not a real keyword because it may depends of the targeted architecture
        SYNTAXE_OPERATOR,

        STRING_LITERAL_RAW,	// @Warning In this case the text is the value of the token instead of a member of the Value union
        STRING_LITERAL,
        NUMERIC_LITERAL_I32,
        NUMERIC_LITERAL_UI32,
        NUMERIC_LITERAL_I64,
        NUMERIC_LITERAL_UI64,
        NUMERIC_LITERAL_F32,
        NUMERIC_LITERAL_F64,
        NUMERIC_LITERAL_REAL,	// means longue double (80bits computations)
    };

    enum class Punctuation : uint8_t
    {
        UNKNOWN,

        // Multiple characters punctuation
        // !!!!! Have to be sort by the number of characters
        // !!!!! to avoid bad forward detection (/** issue when last * can override the /* detection during the forward test)

        // TODO the escape of line return can be handle by the parser by checking if there is no more token after the \ on the line
        //    InhibitedLineReturn,    //    \\n or \\r\n          A backslash that preceed a line return (should be completely skipped)
        LINE_COMMENT,           //    //
        OPEN_BLOCK_COMMENT,     //    /*
        CLOSE_BLOCK_COMMENT,    //    */

        // Mostly in the order on a QWERTY keyboard (symbols making a pair are grouped)
        TILDE,                  //    ~                 Should stay the first of single character symbols
        BACKQUOTE,              //    `
        OPEN_BRACE,             //    {
        CLOSE_BRACE,            //    }
		COLON,					//    :
		SEMICOLON,              //    ;                 comment
        SINGLE_QUOTE,           //    '                 string literal
        DOUBLE_QUOTE,           //    "                 string literal
        COMMA,                  //    ,                 operand separator
		DOT,                    //    .					instruction type suffix

        // White character at end to be able to handle correctly lines that terminate with a separator like semicolon just before a line return
        WHITE_CHARACTER,
        NEW_LINE_CHARACTER
    };

	// @SpeedUp @TODO Est-ce que je dois utiliser
	// #pragma pack(push, 1)
	// #pragma pack(pop)
	// ? pour réduire le sizeof de la structure et donc gagner en perf grace à une réduction des cache-miss
    struct Token
    {
    public:
        union Value
        {
            Punctuation		        punctuation;
            Keyword			        keyword;
            Instruction             instruction;    // Should be the underlying type if I want to be able to switch at runtime the architecture (uint16_t)
            Register                _register;       // Should be the underlying type if I want to be able to switch at runtime the architecture (uint8_t)
            int64_t			        integer;
            uint64_t		        unsigned_integer;
            float			        real_32;
            double			        real_64;
            long double		        real_max;
            fstd::language::string* string;		// polish_string_literal is the method that fill it by using text
        };

        Token_Type			        type;
        fstd::language::string_view file_path;
        fstd::language::string_view text;
        size_t				        line;       // Starting from 1
        size_t				        column;     // Starting from 1
        Value				        value;
    };

    struct Lexer_Data
    {
        fstd::system::Path		        file_path;
        fstd::memory::Array<uint8_t>	file_buffer;
    };

	// @TODO Give the possibility to switch the targetted architecture
	// It should change hash_tables data mostly
	// Store the targetted architecture in Lexer_Data to improve error messages
    void    initialize_lexer();

	void    lex(const fstd::system::Path& path, fstd::memory::Array<Token>& tokens);
    void    lex(const fstd::system::Path& path, fstd::memory::Array<uint8_t>& file_buffer, fstd::memory::Array<Token>& tokens, fstd::memory::Array<f::ASM::Lexer_Data>& lexer_data, Token& file_token);
    void    print(fstd::memory::Array<Token>& tokens);

    inline bool is_white_punctuation(Punctuation punctuation)
    {
        return punctuation >= Punctuation::WHITE_CHARACTER;
    }
}
