#include "lexer_base.hpp"

namespace f
{
    // @TODO remove the use of the initializer list in the Hash_Table
    Hash_Table<uint16_t, Punctuation, Punctuation::UNKNOWN> punctuation_table_2 = {
        // Utf8 characters that use 2 runes
        {punctuation_key_2((uint8_t*)u8"¤"), Punctuation::CURRENCY},
        {punctuation_key_2((uint8_t*)u8"£"), Punctuation::POUND},
        {punctuation_key_2((uint8_t*)u8"§"), Punctuation::SECTION},

        {punctuation_key_2((uint8_t*)"//"),	Punctuation::LINE_COMMENT},
        {punctuation_key_2((uint8_t*)"/*"),	Punctuation::OPEN_BLOCK_COMMENT},
        {punctuation_key_2((uint8_t*)"*/"),	Punctuation::CLOSE_BLOCK_COMMENT},
        {punctuation_key_2((uint8_t*)"->"),	Punctuation::ARROW},
        {punctuation_key_2((uint8_t*)"&&"),	Punctuation::LOGICAL_AND},
        {punctuation_key_2((uint8_t*)"||"),	Punctuation::LOGICAL_OR},
        {punctuation_key_2((uint8_t*)"::"),	Punctuation::DOUBLE_COLON},
        {punctuation_key_2((uint8_t*)".."),	Punctuation::DOUBLE_DOT},
        {punctuation_key_2((uint8_t*)":="),	Punctuation::COLON_EQUAL},
        {punctuation_key_2((uint8_t*)"=="),	Punctuation::EQUALITY_TEST},
        {punctuation_key_2((uint8_t*)"!="),	Punctuation::DIFFERENCE_TEST},
        {punctuation_key_2((uint8_t*)"\\\""),	Punctuation::ESCAPED_DOUBLE_QUOTE},
    };

    Hash_Table<uint8_t, Punctuation, Punctuation::UNKNOWN> punctuation_table_1 = {
        // White characters (aren't handle for an implicit skip/separation between tokens)
        {' ', Punctuation::WHITE_CHARACTER},       // space
        {'\t', Punctuation::WHITE_CHARACTER},      // horizontal tab
        {'\v', Punctuation::WHITE_CHARACTER},      // vertical tab
        {'\f', Punctuation::WHITE_CHARACTER},      // feed
        {'\r', Punctuation::WHITE_CHARACTER},      // carriage return
        {'\n', Punctuation::NEW_LINE_CHARACTER},   // newline
        {'~', Punctuation::TILDE},
        {'`', Punctuation::BACKQUOTE},
        {'!', Punctuation::BANG},
        {'@', Punctuation::AT},
        {'#', Punctuation::HASH},
        {'$', Punctuation::DOLLAR},
        {'%', Punctuation::PERCENT},
        {'^', Punctuation::CARET},
        {'&', Punctuation::AMPERSAND},
        {'*', Punctuation::STAR},
        {'(', Punctuation::OPEN_PARENTHESIS},
        {')', Punctuation::CLOSE_PARENTHESIS},
        {'-', Punctuation::DASH},
        {'+', Punctuation::PLUS},
        {'=', Punctuation::EQUALS},
        {'{', Punctuation::OPEN_BRACE},
        {'}', Punctuation::CLOSE_BRACE},
        {'[', Punctuation::OPEN_BRACKET},
        {']', Punctuation::CLOSE_BRACKET},
        {':', Punctuation::COLON},
        {';', Punctuation::SEMICOLON},
        {'\'', Punctuation::SINGLE_QUOTE},
        {'"', Punctuation::DOUBLE_QUOTE},
        {'|', Punctuation::PIPE},
        {'/', Punctuation::SLASH},
        {'\\', Punctuation::BACKSLASH},
        {'<', Punctuation::LESS},
        {'>', Punctuation::GREATER},
        {',', Punctuation::COMMA},
        {'.', Punctuation::DOT},
        {'?', Punctuation::QUESTION_MARK}
    };
}
