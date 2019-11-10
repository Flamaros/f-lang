#pragma once

#include <vector>
#include <string_view>

/*
Notice:
 - Inspired by Dlang lexer (https://dlang.org/spec/lex.html#IntegerLiteral), flang lexer don't support
   long double (80 bits) type and imaginary numbers. An other difference is that for prefixes and suffixes
   flang lexer only support lower case in exception of L that looks too closely to 1 in lower case.
 - '-' character is an operator, so it can't be used to declare identifier (variables, functions,...).
   Also numeric_literal never contains negative number, this is the role of the parser do desembiguate
   if this character is used as unary or binary operator and eventually to modify the token value.
*/

namespace f
{
	enum class Token_Type : uint8_t
	{
		identifier,	/// Every word that isn't a part of the language
		keyword,
		syntaxe_operator,
		string_literal,
		numeric_literal_i32,
		numeric_literal_ui32,
		numeric_literal_i64,
		numeric_literal_ui64,
		numeric_literal_f32,
		numeric_literal_f64,
		numeric_literal_real,	// means longue double (80bits computations)
	};

    enum class Punctuation : uint8_t
    {
        unknown,

        // Multiple characters punctuation
        // !!!!! Have to be sort by the number of characters
        // !!!!! to avoid bad forward detection (/** issue when last * can override the /* detection during the forward test)

    // TODO the escape of line return can be handle by the parser by checking if there is no more token after the \ on the line
    //    InhibitedLineReturn,    //    \\n or \\r\n          A backslash that preceed a line return (should be completely skipped)
        line_comment,           //    //
        open_block_comment,     //    /*
        close_block_comment,    //    */
        arrow,                  //    ->
        logical_and,            //    &&
        logical_or,             //    ||
        double_colon,           //    ::                 Used for function or struct declarations
		double_dot,             //    ..                 Used for ranges
		equality_test,          //    ==
        difference_test,        //    !=
		escaped_double_quote,	//	  \"				To ease the detection of the end of the string litteral by avoiding the necessary to check it in an other way
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
        tilde,                  //    ~                  Should stay the first of single character symbols
        backquote,              //    `
        bang,                   //    !
        at,                     //    @
        hash,                   //    #
        dollar,                 //    $
        percent,                //    %
        caret,                  //    ^
        ampersand,              //    &					 bitwise and
        star,                   //    *
        open_parenthesis,       //    (
        close_parenthesis,      //    )
        underscore,             //    _
        dash,                   //    -
        plus,                   //    +
        equals,                 //    =
        open_brace,             //    {
        close_brace,            //    }
        open_bracket,           //    [
        close_bracket,          //    ]
        colon,                  //    :                  Used in ternaire expression
        semicolon,              //    ;
        single_quote,           //    '
        double_quote,           //    "
        pipe,                   //    |					 bitwise or
        slash,                  //    /
        backslash,              //    '\'
        less,                   //    <
        greater,                //    >
        comma,                  //    ,
        dot,                    //    .
        question_mark,          //    ?

        // White character at end to be able to handle correctly lines that terminate with a separator like semicolon just before a line return
        white_character,
        new_line_character
    };

    enum class Keyword : uint8_t // @Warning I use an _ as prefix to avoid collisions with cpp keywords
    {
        _unknown,

        // Special keywords
        _import,
        _enum,
        _struct,
        _typedef,
        K_inline,
        _static,
        _fn,
        _true,
        _false,
		_nullptr,
		_immutable,

        // Reserved for futur usage
        _public,
        _protected,
        _private,

        // Control flow
        _if,
        _else,
        _do,
        _while,
        _for,
        _foreach,
        _switch,
        _case,
        _default,
        _final,
        _return,
        _exit,

        // Types
        _bool,
        _i8,
        _ui8,
        _i16,
        _ui16,
        _i32,
        _ui32,
        _i64,
        _ui64,
        _f32,
        _f64,
        _string,
        // @TODO @Critical add types that have the size of ptr (like size_t and ptrdiff_t,...)

		// Special keywords (interpreted by the lexer)
		// They will change the resulting token
		special_file,				// replaced by a string litteral that contains current file name
		special_full_path_file,		// replaced by a string litteral that contains current file absolut path
		special_line,				// replaced by a number litteral that contains the current line value
		special_module,				// replaced by a string litteral that contains current module name (equivalent to file for the moment)
		special_eof,				// stop the lexer
		special_compiler_vendor,	// replaced by a string litteral that contains the vendor of running compiler
		special_compiler_version,	// replaced by a string litteral that contains the version of running compiler
    };

	struct Token
	{
	private:
		friend bool operator ==(const Token& lhs, const Token& rhs);

	public:
		union Value
		{
			Punctuation	punctuation;
			Keyword		keyword;
			int64_t		integer;
			uint64_t	unsigned_integer;
			float		real_32;
			double		real_64;
			long double	real_max;
		};

		Token_Type			type;
		std::string_view	text;
		size_t				line;       // Starting from 1
		size_t				column;     // Starting from 1
		Value				value;
	};
	
	inline bool operator ==(const Token& lhs, const Token& rhs)
	{
		return lhs.text == rhs.text;
	}

	void    tokenize(const std::string& text, std::vector<Token>& tokens);
}
