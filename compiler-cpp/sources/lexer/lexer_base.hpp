#pragma once

#include "hash_table.hpp"

#include <fstd/language/string.hpp>
#include <fstd/language/string_view.hpp>

#include <fstd/stream/array_stream.hpp>

#include <fstd/system/path.hpp>

namespace f
{
    enum class Token_Type : uint8_t
    {
        // @Warning don't change the order of types
        // Or take care to update functions like is_literal

        UNKNOWN,
        IDENTIFIER,	/// Every word that isn't a part of the language
        KEYWORD,
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
        ARROW,                  //    ->
        LOGICAL_AND,            //    &&
        LOGICAL_OR,             //    ||
        DOUBLE_COLON,           //    ::                 Used for function or struct declarations
        DOUBLE_DOT,             //    ..                 Used for ranges
        COLON_EQUAL,            //    :=                 Used for variable declarations with type inference
        EQUALITY_TEST,          //    ==
        DIFFERENCE_TEST,        //    !=
        ESCAPED_DOUBLE_QUOTE,	//	  \"				To ease the detection of the end of the string litteral by avoiding the necessary to check it in an other way
        // Not sure that must be detected as one token instead of multiples (especially <<, >>, <<= and >>=) because of templates
    //    LeftShift,              //    <<
    //    RightShift,             //    >>
    //    AdditionAssignment,     //    +=
    //    SubstractionAssignment, //    -=
    //    MultiplicationAssignment, //    *=
    //    DivisionAssignment,     //    /=
    //    DivisionAssignment,     //    %=
    //    DivisionAssignment,     //    |=
    //    DivisionAssignment,     //    &=
    //    DivisionAssignment,     //    ^=
    //    DivisionAssignment,     //    <<=
    //    DivisionAssignment,     //    >>=

        // Mostly in the order on a QWERTY keyboard (symbols making a pair are grouped)
        TILDE,                  //    ~                  Should stay the first of single character symbols
        BACKQUOTE,              //    `
        BANG,                   //    !
        AT,                     //    @                 pointer unreferencing operator
        HASH,                   //    #
        DOLLAR,                 //    $
        POUND,                  //    £
        PERCENT,                //    %
        CARET,                  //    ^
        AMPERSAND,              //    &					 bitwise and
        STAR,                   //    *
        SECTION,                //    §                  pointer symbol
        CURRENCY,               //    ¤
        OPEN_PARENTHESIS,       //    (
        CLOSE_PARENTHESIS,      //    )
        DASH,                   //    -
        PLUS,                   //    +
        EQUALS,                 //    =
        OPEN_BRACE,             //    {
        CLOSE_BRACE,            //    }
        OPEN_BRACKET,           //    [
        CLOSE_BRACKET,          //    ]
        COLON,                  //    :                  Used in ternaire expression
        SEMICOLON,              //    ;
        SINGLE_QUOTE,           //    '
        DOUBLE_QUOTE,           //    "
        PIPE,                   //    |					 bitwise or
        SLASH,                  //    /
        BACKSLASH,              //    '\'
        LESS,                   //    <
        GREATER,                //    >
        COMMA,                  //    ,
        DOT,                    //    .
        QUESTION_MARK,          //    ?

        // White character at end to be able to handle correctly lines that terminate with a separator like semicolon just before a line return
        WHITE_CHARACTER,
        NEW_LINE_CHARACTER
    };

    template<typename Keyword>
    struct Token
    {
    public:
        union Value
        {
            Punctuation		        punctuation;
            Keyword			        keyword;
            int64_t			        integer;
            uint64_t		        unsigned_integer;
            float			        real_32;
            double			        real_64;
            long double		        real_max;
            fstd::language::string* string;
        };

        Token_Type			        type;
        fstd::language::string_view file_path;
        fstd::language::string_view text;
        size_t				        line;       // Starting from 1
        size_t				        column;     // Starting from 1
        Value				        value;
    };

    extern Hash_Table<uint16_t, Punctuation, Punctuation::UNKNOWN> punctuation_table_2;
    extern Hash_Table<uint8_t, Punctuation, Punctuation::UNKNOWN> punctuation_table_1;
    extern fstd::language::string_view keyword_invalid_key;

    /// Return a key for the punctuation of 2 characters
    constexpr uint16_t punctuation_key_2(const uint8_t* str)
    {
        return ((uint16_t)str[0] << 8) | (uint16_t)str[1];
    }

    inline bool is_literal(Token_Type token_type) {
        return token_type >= Token_Type::STRING_LITERAL_RAW && token_type <= Token_Type::NUMERIC_LITERAL_REAL;
    }

    inline void peek(fstd::stream::Array_Stream<uint8_t>& stream, int& current_column)
    {
        fstd::stream::peek<uint8_t>(stream);
        current_column++;
    }

    inline void skip(fstd::stream::Array_Stream<uint8_t>& stream, size_t size, int& current_column)
    {
        fstd::stream::skip<uint8_t>(stream, size);
        current_column += (int)size;
    }

    inline uint32_t keyword_key(const fstd::language::string_view& str)
    {
        // @TODO test with a crc32 implementation (that could be better to reduce number of collisions instead of this weird thing)

        fstd::core::Assert(fstd::language::get_string_size(str) > 0);

        if (fstd::language::get_string_size(str) == 1) {
            return (uint32_t)1 << 8 | (uint32_t)str.ptr[0];
        }
        else if (fstd::language::get_string_size(str) == 2) {
            return (uint32_t)2 << 24 | (uint32_t)str.ptr[0] << 8 | (uint32_t)str.ptr[1];
        }
        else if (fstd::language::get_string_size(str) < 5) {
            return (uint32_t)str.size << 24 | ((uint32_t)(str.ptr[0]) << 16) | ((uint32_t)(str.ptr[1]) << 8) | (uint32_t)str.ptr[2];
        }
        else {
            return (uint32_t)str.size << 24 | ((uint32_t)(str.ptr[0]) << 16) | ((uint32_t)(str.ptr[2]) << 8) | (uint32_t)str.ptr[4];
        }
    }

    inline bool is_digit(char character)
    {
        if (character >= '0' && character <= '9') {
            return true;
        }
        return false;
    }
}
