#pragma once

#include "lexer_base.hpp"

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
    enum class Keyword : uint8_t
    {
        UNKNOWN,

        // Special keywords
        IMPORT,
        ENUM,
        STRUCT,
        UNION,
        ALIAS, // @TODO remove it
        INLINE,
        STATIC,
        TRUE,
        FALSE,
		NULLPTR,
		IMMUTABLE,
        USING,
        NEW,
        DELETE,

        // Reserved for futur usage
        PUBLIC,
        PROTECTED,
        PRIVATE,
        MODULE,

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

        // Types (@Warning BOOL should stay first, and TYPE latest)
        VOID,
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
        STRING_VIEW,
        TYPE,   // For variables that store a Type (function, ui32, f32,...)
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

    struct Lexer_Data
    {
        fstd::system::Path		        file_path;
        fstd::memory::Array<uint8_t>	file_buffer;
    };

    void    initialize_lexer();
	void    lex(const fstd::system::Path& path, fstd::memory::Array<Token<Keyword>>& tokens);
    void    lex(const fstd::system::Path& path, fstd::memory::Array<uint8_t>& file_buffer, fstd::memory::Array<Token<Keyword>>& tokens, fstd::memory::Array<f::Lexer_Data>& lexer_data, Token<Keyword>& file_token);
    void    print(fstd::memory::Array<Token<Keyword>>& tokens);

    inline bool is_a_basic_type(Keyword keyword) {
        return keyword >= Keyword::VOID && keyword <= Keyword::TYPE;
    }

    inline bool is_white_punctuation(Punctuation punctuation)
    {
        return punctuation >= Punctuation::WHITE_CHARACTER;
    }
}
