#pragma once

#include <fstd/language/string.hpp>
#include <fstd/language/string_view.hpp>

#include <fstd/system/path.hpp>

#include <fstd/memory/array.hpp>

#include <fstd/core/assert.hpp>

/*
Notice:
 - Inspired by Dlang lexer (https://dlang.org/spec/lex.html#IntegerLiteral), flang lexer don't support
   long double (80 bits) type and imaginary numbers. An other difference is that for prefixes and suffixes
   flang lexer only support lower case in exception of L that looks too closely to 1 in lower case.
 - '-' character is an operator, so it can't be used to declare identifier (variables, functions,...).
   Also numeric_literal never contains negative number, this is the role of the parser do desembiguate
   if this character is used as unary or binary operator and eventually to modify the token value.
*/

#if defined(TRUE) || defined(FALSE)
#	error "TRUE or FALSE should not be defined before including this file"
#endif

namespace f
{
	enum class Token_Type : uint8_t
	{
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
        AT,                     //    @
        HASH,                   //    #
        DOLLAR,                 //    $
        PERCENT,                //    %
        CARET,                  //    ^
        AMPERSAND,              //    &					 bitwise and
        STAR,                   //    *
        OPEN_PARENTHESIS,       //    (
        CLOSE_PARENTHESIS,      //    )
        UNDERSCORE,             //    _
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

    enum class Keyword : uint8_t
    {
        UNKNOWN,

        // Special keywords
        IMPORT,
        ENUM,
        STRUCT,
        TYPEDEF,
        INLINE,
        STATIC,
        TRUE,
        FALSE,
		NULLPTR,
		IMMUTABLE,

        // Reserved for futur usage
        PUBLIC,
        PROTECTED,
        PRIVATE,

        // Control flow
        IF,
        ELSE,
        DO,
        WHILE,
        FOR,
        FOREACH,
        SWITCH,
        CASE,
        DEFAULT,
        FINAL,
        RETURN,
        EXIT,

        // Types
        BOOL,
        I8,
        UI8,
        I16,
        UI16,
        I32,
        UI32,
        I64,
        UI64,
        F32,
        F64,
        STRING,
        // @TODO @Critical add types that have the size of ptr (like size_t and ptrdiff_t,...)

		// Special keywords (interpreted by the lexer)
		// They will change the resulting token
		SPECIAL_FILE,				// replaced by a string litteral that contains current file name
		SPECIAL_FULL_PATH_FILE,		// replaced by a string litteral that contains current file absolut path
		SPECIAL_LINE,				// replaced by a number litteral that contains the current line value
		SPECIAL_MODULE,				// replaced by a string litteral that contains current module name (equivalent to file for the moment)
		SPECIAL_EOF,				// stop the lexer
		SPECIAL_COMPILER_VENDOR,	// replaced by a string litteral that contains the vendor of running compiler
		SPECIAL_COMPILER_VERSION,	// replaced by a string litteral that contains the version of running compiler
    };

	struct Token
	{
	private:
		friend bool operator ==(const Token& lhs, const Token& rhs);

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
        fstd::language::string_view text;
		size_t				        line;       // Starting from 1
		size_t				        column;     // Starting from 1
		Value				        value;
	};
	
	inline bool operator ==(const Token& lhs, const Token& rhs)
	{
        fstd::core::Assert(false);
        return false;
//		return lhs.text == rhs.text;
	}

	bool    lex(const fstd::system::Path& path, fstd::memory::Array<Token>& tokens);
}
