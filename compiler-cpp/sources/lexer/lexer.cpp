#include "lexer.hpp"

#include "hash_table.hpp"
#include "keyword_hash_table.hpp"

#include "../globals.hpp"

#include <fstd/core/logger.hpp>
#include <fstd/core/string_builder.hpp>

#include <fstd/system/file.hpp>
#include <fstd/system/path.hpp>

#include <fstd/stream/memory_stream.hpp>

#include <fstd/language/defer.hpp>
#include <fstd/language/flags.hpp>
#include <fstd/language/string.hpp>

#undef max
#include <tracy/Tracy.hpp>

using namespace f;

using namespace fstd;
using namespace fstd::core;

static const size_t    tokens_length_heuristic = 5;

/// Return a key for the punctuation of 2 characters
constexpr static uint16_t punctuation_key_2(const uint8_t* str)
{
    return ((uint16_t)str[0] << 8) | (uint16_t)str[1];
}

// @Warning
// Following Hash_Table should be filled at compile time not runtime.
// I actually didn't check how it goes in C++, but in f-lang it should be
// explicite than there is no initialization at runtime.
//
// Flamaros - 15 february 2020

// @TODO
// Check if naive chain of if - else if isn't faster than those Hash_Table for punctuation.
// For punctuation with many characters we can also make the tests of other caracters only if
// previous can be followed by an other one.
//
// Flamaros - 17 february 2020

// @TODO remove the use of the initializer list in the Hash_Table
static Hash_Table<uint16_t, Punctuation, Punctuation::UNKNOWN> punctuation_table_2 = {
    {punctuation_key_2((uint8_t*)"//"),	Punctuation::LINE_COMMENT},
    {punctuation_key_2((uint8_t*)"/*"),	Punctuation::OPEN_BLOCK_COMMENT},
    {punctuation_key_2((uint8_t*)"*/"),	Punctuation::CLOSE_BLOCK_COMMENT},
    {punctuation_key_2((uint8_t*)"->"),	Punctuation::ARROW},
    {punctuation_key_2((uint8_t*)"&&"),	Punctuation::LOGICAL_AND},
    {punctuation_key_2((uint8_t*)"||"),	Punctuation::LOGICAL_OR},
    {punctuation_key_2((uint8_t*)"::"),	Punctuation::DOUBLE_COLON},
	{punctuation_key_2((uint8_t*)".."),	Punctuation::DOUBLE_DOT},
	{punctuation_key_2((uint8_t*)"=="),	Punctuation::EQUALITY_TEST},
    {punctuation_key_2((uint8_t*)"!="),	Punctuation::DIFFERENCE_TEST},
	{punctuation_key_2((uint8_t*)"\\\""),	Punctuation::ESCAPED_DOUBLE_QUOTE},
};

static Hash_Table<uint8_t, Punctuation, Punctuation::UNKNOWN> punctuation_table_1 = {
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
//  {'_', Token::underscore},
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

static inline bool is_white_punctuation(Punctuation punctuation)
{
    return punctuation >= Punctuation::WHITE_CHARACTER;
}

static uint32_t keyword_key(const language::string_view& str)
{
    // @TODO test with a crc32 implementation (that call be better to reduce number of collision instead of this weird thing)

    core::Assert(language::get_string_size(str) > 0);

    if (language::get_string_size(str) == 1) {
        return (uint32_t)1 << 8 | (uint32_t)str.ptr[0];
    }
    else if (language::get_string_size(str) == 2) {
        return (uint32_t)2 << 24 | (uint32_t)str.ptr[0] << 8 | (uint32_t)str.ptr[1];
    }
    else if (language::get_string_size(str) < 5) {
        return (uint32_t)str.size << 24 | ((uint32_t)(str.ptr[0]) << 16) | ((uint32_t)(str.ptr[1]) << 8) | (uint32_t)str.ptr[2];
    }
    else {
        return (uint32_t)str.size << 24 | ((uint32_t)(str.ptr[0]) << 16) | ((uint32_t)(str.ptr[2]) << 8) | (uint32_t)str.ptr[4];
    }
}

static language::string_view keyword_invalid_key;

// @TODO Can't we reduce the size key?
// To avoid collisions, we may have to find a better way to generate a hash than simply encode the keyword size with the first
// two characters.
// @SpeedUp
//
// Flamaros - 19 february 2020
static Keyword_Hash_Table<uint32_t, language::string_view, Keyword, &keyword_invalid_key, Keyword::UNKNOWN>  keywords;

static inline Keyword is_keyword(const language::string_view& text)
{
    return keywords.find(keyword_key(text), text);
}

static inline bool is_digit(char character)
{
	if (character >= '0' && character <= '9') {
		return true;
	}
	return false;
}

// @TODO remplace it by a nested inlined function in f-lang
#define INSERT_KEYWORD(KEY, VALUE) \
    { \
        language::string_view   str_view; \
        language::assign(str_view, (uint8_t*)(KEY)); \
        keywords.insert(keyword_key(str_view), str_view, (Keyword::VALUE)); \
    }

void f::initialize_lexer()
{
    keywords.set_key_matching_function(&language::are_equals);

    INSERT_KEYWORD("import", IMPORT);

    INSERT_KEYWORD("enum", ENUM);
    INSERT_KEYWORD("struct", STRUCT);
    INSERT_KEYWORD("typedef", TYPEDEF);
    INSERT_KEYWORD("inline", INLINE);
    INSERT_KEYWORD("static", STATIC);
    INSERT_KEYWORD("true", TRUE);
    INSERT_KEYWORD("false", FALSE);
    INSERT_KEYWORD("nullptr", NULLPTR);
    INSERT_KEYWORD("immutable", IMMUTABLE);

        // Control flow
    INSERT_KEYWORD("if", IF);
    INSERT_KEYWORD("else", ELSE);
    INSERT_KEYWORD("do", DO);
    INSERT_KEYWORD("while", WHILE);
    INSERT_KEYWORD("for", FOR);
    INSERT_KEYWORD("foreach", FOREACH);
    INSERT_KEYWORD("switch", SWITCH);
    INSERT_KEYWORD("case", CASE);
    INSERT_KEYWORD("default", DEFAULT);
    INSERT_KEYWORD("final", FINAL);
    INSERT_KEYWORD("return", RETURN);
    INSERT_KEYWORD("exit", EXIT);

        // Reserved for futur usage
    INSERT_KEYWORD("public,", PUBLIC);
    INSERT_KEYWORD("protected,", PROTECTED);
    INSERT_KEYWORD("private,", PRIVATE);

        // Types
    INSERT_KEYWORD("bool", BOOL);
    INSERT_KEYWORD("i8", I8);
    INSERT_KEYWORD("ui8", UI8);
    INSERT_KEYWORD("i16", I16);
    INSERT_KEYWORD("ui16", UI16);
    INSERT_KEYWORD("i32", I32);
    INSERT_KEYWORD("ui32", UI32);
    INSERT_KEYWORD("i64", I64);
    INSERT_KEYWORD("ui64", UI64);
    INSERT_KEYWORD("f32", F32);
    INSERT_KEYWORD("f64", F64);
    INSERT_KEYWORD("string", STRING);
    INSERT_KEYWORD("string_view", STRING_VIEW);

        // Special keywords (interpreted by the lexer)
    INSERT_KEYWORD("__FILE__", SPECIAL_FILE);
    INSERT_KEYWORD("__FILE_FULL_PATH__",  SPECIAL_FULL_PATH_FILE);
    INSERT_KEYWORD("__LINE__", SPECIAL_LINE);
    INSERT_KEYWORD("__MODULE__", SPECIAL_MODULE);
    INSERT_KEYWORD("__EOF__", SPECIAL_EOF);
    INSERT_KEYWORD("__VENDOR__", SPECIAL_COMPILER_VENDOR);
    INSERT_KEYWORD("__VERSION__", SPECIAL_COMPILER_VERSION);

    core::log(*globals.logger, Log_Level::verbose, "[lexer] keywords hash table: size: %d bytes - nb_used_buckets: %d - nb_collisions: %d\n", keywords.compute_used_memory_in_bytes(), keywords.nb_used_buckets(), keywords.nb_collisions());
}

static inline void peek(stream::Memory_Stream& stream, int& current_column)
{
    stream::peek(stream);
    current_column++;
}

static inline void skip(stream::Memory_Stream& stream, size_t size, int& current_column)
{
    stream::skip(stream, size);
    current_column += (int)size;
}

static inline void polish_string_literal(f::Token& token)
{
    fstd::core::Assert(token.type == Token_Type::STRING_LITERAL);

    language::string* string = (language::string*)system::allocate(sizeof(language::string));
    init(*string);

    memory::reserve_array(string->buffer, language::get_string_size(token.text));

    size_t      position = 0;
    size_t      literal_length = 0;
    uint8_t*    output = (uint8_t*)&string[0];
    size_t      token_length = language::get_string_size(token.text);

    while (position < token_length)
    {
        if (language::to_utf8(token.text)[position] == '\\') {
            position++;
            if (language::to_utf8(token.text)[position] == 'n') {
                output[literal_length] = '\n';
            }
            else if (language::to_utf8(token.text)[position] == 'r') {
                output[literal_length] = '\r';
            }
            else if (language::to_utf8(token.text)[position] == 't') {
                output[literal_length] = '\t';
            }
            else if (language::to_utf8(token.text)[position] == 'v') {
                output[literal_length] = '\v';
            }
            else if (language::to_utf8(token.text)[position] == '\\') {
                output[literal_length] = '\\';
            }
            literal_length++;
        }
        else {
            output[literal_length] = language::to_utf8(token.text)[position];
            literal_length++;
        }
        position++;
    }

    language::resize(*string, literal_length);
    token.value.string = string;
}

bool f::lex(const system::Path& path, memory::Array<Token>& tokens)
{
	ZoneScopedNC("f::lex",  0x1b5e20);

    Lexer_Data              lexer_data;
    system::File	        file;
    Token                   file_token;
    stream::Memory_Stream	stream;
    size_t	                nb_tokens_prediction = 0;
    uint64_t                file_size;
    language::string_view   current_view;
    int					    current_line = 1;
    int					    current_column = 1;

    if (system::open_file(file, path, system::File::Opening_Flag::READ) == false) {
        file_token.type = Token_Type::UNKNOWN;
        file_token.file_path = system::to_string(path);
        file_token.line = 0;
        file_token.column = 0;
        report_error(Compiler_Error::error, file_token, "Failed to open source file.");
    }

    defer{ system::close_file(file); };

    file_size = system::get_file_size(file);

    lexer_data.file_buffer = system::initiate_get_file_content_asynchronously(file);
    memory::array_push_back(globals.lexer_data, lexer_data);

    stream::initialize_memory_stream(stream, lexer_data.file_buffer);

    if (stream::is_eof(stream) == true) {
        return true;
    }

    // @Warning
    //
    // We read the file in background asynchronously, so be careful to not relly on file states.
    // Instead prefer use the stream to get correct states.
    //
    // Flamaros - 01 february 2020

    nb_tokens_prediction = file_size / tokens_length_heuristic;

    memory::reserve_array(tokens, nb_tokens_prediction);

    language::assign(current_view, stream::get_pointer(stream), 0);

    wait_for_availabe_asynchronous_content(file, stream::get_position(stream) + 4);
    bool has_utf8_boom = stream::is_uft8_bom(stream, true);

    if (has_utf8_boom == false) {
        report_error(Compiler_Error::warning, file_token, "This file doens't have a UTF8 BOM");
    }

    while (stream::is_eof(stream) == false)
    {
        Punctuation punctuation;
        uint8_t     current_character;
            
        current_character = stream::get(stream);

        punctuation = punctuation_table_1[current_character];
        if (punctuation != Punctuation::UNKNOWN) { // Punctuation to analyse
            if (is_white_punctuation(punctuation)) {    // Punctuation to ignore
                if (punctuation == Punctuation::NEW_LINE_CHARACTER) {
                    current_line++;
                    current_column = 0; // @Warning 0 because the will be incremented just after
                }

                peek(stream, current_column);
            }
            else {
                Token       token;
                Punctuation punctuation_2 = Punctuation::UNKNOWN;

                token.file_path = system::to_string(path);
                token.line = current_line;
                token.column = current_column;
                    
                language::assign(current_view, stream::get_pointer(stream), 0);

                if (stream::get_remaining_size(stream) >= 2) {
                    punctuation_2 = punctuation_table_2[punctuation_key_2(stream::get_pointer(stream))];
                }

                if (punctuation_2 == Punctuation::LINE_COMMENT) {
                    while (stream::is_eof(stream) == false)
                    {
                        uint8_t     current_character;

                        current_character = stream::get(stream);
                        if (current_character == '\n') {
                            break;
                        }
                        peek(stream, current_column);   // @Warning We don't peek the '\n' character (it will be peeked later for the line count increment)
                    }
                }
                else if (punctuation_2 == Punctuation::OPEN_BLOCK_COMMENT) {
                    bool    comment_block_closed = false;
                    while (stream::is_eof(stream) == false)
                    {
                        if (stream::get_remaining_size(stream) >= 2) {
                            punctuation_2 = punctuation_table_2[punctuation_key_2(stream::get_pointer(stream))];
                        }

                        if (punctuation_2 == Punctuation::CLOSE_BLOCK_COMMENT) {
                            skip(stream, 2, current_column);
                            comment_block_closed = true;
                            break;
                        }

                        peek(stream, current_column);
                    }

                    if (comment_block_closed == false) {
                        report_error(Compiler_Error::error, token, "Multiline comment block was not closed.");
                    }
                }
				else if (punctuation == Punctuation::DOUBLE_QUOTE) {
                    peek(stream, current_column);

                    bool        string_closed = false;
                    uint8_t*    string_literal = stream::get_pointer(stream);
                    size_t      string_size = 0;

                    while (stream::is_eof(stream) == false)
                    {
                        uint8_t     current_character;

                        current_character = stream::get(stream);
                        if (punctuation_table_1[current_character] == Punctuation::DOUBLE_QUOTE) { // @TODO @SpeedUp we can just check the character here directly
                            peek(stream, current_column);
                            string_closed = true;
                            break;
                        }

                        peek(stream, current_column);
                        string_size++;
                    }

                    if (string_closed == false) {
                        report_error(Compiler_Error::error, token, "String literal was not closed.");
                    }
                    else {
                        language::assign(current_view, string_literal, string_size);
                        token.text = current_view;
                        token.type = Token_Type::STRING_LITERAL;
                        polish_string_literal(token);

                        memory::array_push_back(tokens, token);
                    }
                }
				else if (punctuation == Punctuation::SINGLE_QUOTE) {
                    peek(stream, current_column);
                    // @TODO single character, can be the same as string literal???
				}
				else if (punctuation == Punctuation::BACKQUOTE) {
                    peek(stream, current_column);

                    bool        raw_string_closed = false;
                    uint8_t* string_literal = stream::get_pointer(stream);
                    size_t      string_size = 0;

                    while (stream::is_eof(stream) == false)
                    {
                        uint8_t     current_character;

                        current_character = stream::get(stream);
                        if (punctuation_table_1[current_character] == Punctuation::SINGLE_QUOTE) { // @TODO @SpeedUp we can just check the character here directly
                            peek(stream, current_column);
                            raw_string_closed = true;
                            break;
                        }

                        peek(stream, current_column);
                        string_size++;
                    }

                    if (raw_string_closed == false) {
                        report_error(Compiler_Error::error, token, "Raw string literal was not closed.");
                    }
                    else {
                        language::assign(current_view, string_literal, string_size);
                        token.text = current_view;
                        token.type = Token_Type::STRING_LITERAL_RAW;

                        memory::array_push_back(tokens, token);
                    }
                }
				else {
					token.type = Token_Type::SYNTAXE_OPERATOR;

					if (punctuation_2 != Punctuation::UNKNOWN) {
						language::assign(current_view, stream::get_pointer(stream), 2);
						token.text = current_view;
						token.value.punctuation = punctuation_2;
						skip(stream, 2, current_column);
					}
                    else {
                        language::assign(current_view, stream::get_pointer(stream), 1);
                        token.text = current_view;
                        token.value.punctuation = punctuation;
                        skip(stream, 1, current_column);
                    }

                    memory::array_push_back(tokens, token);
                }
            }
        }
/*            else if (is_digit(current_character)) { // Will be a numeric literal

            // @TODO
            //
            // Be sure to check that the numeric literal is followed by a white punctuation.
            //
            // Flamaros - 02 february 2020

        }*/
        else {  // Will be an identifier
            Token   token;

            token.file_path = system::to_string(path);
            token.line = current_line;
            token.column = current_column;
                    
            language::assign(current_view, stream::get_pointer(stream), 0);
            while (stream::is_eof(stream) == false)
            {
                uint8_t     current_character;

                current_character = stream::get(stream);

                punctuation = punctuation_table_1[current_character];
                if (punctuation == Punctuation::UNKNOWN) {  // @Warning any kind of punctuation stop the definition of an identifier
                    peek(stream, current_column);
                    language::resize(current_view, language::get_string_size(current_view) + 1);

                    token.type = Token_Type::IDENTIFIER;

                    token.text = current_view;
                }
                else {
                    break;
                }
            }

            token.value.keyword = is_keyword(token.text);
            if (token.value.keyword != Keyword::UNKNOWN) {
                token.type = Token_Type::KEYWORD;
            }
            memory::array_push_back(tokens, token);
        }
    }

	if (nb_tokens_prediction < memory::get_array_size(tokens)) {
		log(*globals.logger, Log_Level::warning, "[lexer] Wrong token number prediction. Predicted :%d - Nb tokens: %d - Nb tokens/byte: %.3f",  nb_tokens_prediction, memory::get_array_size(tokens), (float)memory::get_array_size(tokens) / (float)file_size);
	}

    log(*globals.logger, Log_Level::verbose, "[lexer] keywords hash table: nb_find_calls: %d - nb_collisions_in_find: %d\n", keywords.nb_find_calls(), keywords.nb_collisions_in_find());

	return true;
}

void f::print(fstd::memory::Array<Token>& tokens)
{
    String_Builder  string_builder;

    defer { free_buffers(string_builder); };

    if (memory::get_array_size(tokens)) {
        print_to_builder(string_builder, "--- tokens list of: ");
        print_to_builder(string_builder, tokens[0].file_path);
        print_to_builder(string_builder, " ---\n");
    }

    // @TODO add colors on types,...

    for (size_t i = 0; i < memory::get_array_size(tokens); i++)
    {
        switch (tokens[i].type)
        {
        case Token_Type::UNKNOWN:
            print_to_builder(string_builder, "%d, %d - UNKNOWN\033[0m: %v", tokens[i].line, tokens[i].column, tokens[i].text);
            break;
        case Token_Type::IDENTIFIER:
            print_to_builder(string_builder, "%d, %d - IDENTIFIER\033[0m: %v", tokens[i].line, tokens[i].column, tokens[i].text);
            break;
        case Token_Type::KEYWORD:
            print_to_builder(string_builder, "%d, %d - \033[38;5;12mKEYWORD\033[0m: \033[38;5;12m%v\033[0m", tokens[i].line, tokens[i].column, tokens[i].text);
            break;
        case Token_Type::SYNTAXE_OPERATOR:
            print_to_builder(string_builder, "%d, %d - \033[38;5;10mSYNTAXE_OPERATOR\033[0m: \033[38;5;10m%v\033[0m", tokens[i].line, tokens[i].column, tokens[i].text);
            break;
        case Token_Type::STRING_LITERAL_RAW:
            print_to_builder(string_builder, "%d, %d - \033[38;5;3mSTRING_LITERAL_RAW\033[0m: \033[38;5;3m%v\033[0m", tokens[i].line, tokens[i].column, tokens[i].text);
            break;
        case Token_Type::STRING_LITERAL:
            print_to_builder(string_builder, "%d, %d - \033[38;5;3mSTRING_LITERAL\033[0m: \033[38;5;3m\"%v\"\033[0m", tokens[i].line, tokens[i].column, tokens[i].text);
            break;
        case Token_Type::NUMERIC_LITERAL_I32:
            print_to_builder(string_builder, "%d, %d - \033[38;5;14mNUMERIC_LITERAL_I32\033[0m: \033[38;5;14m'%v'\033[0m", tokens[i].line, tokens[i].column, tokens[i].text);
            break;
        case Token_Type::NUMERIC_LITERAL_UI32:
            print_to_builder(string_builder, "%d, %d - \033[38;5;14mNUMERIC_LITERAL_UI32\033[0m: \033[38;5;14m%v\033[0m", tokens[i].line, tokens[i].column, tokens[i].text);
            break;
        case Token_Type::NUMERIC_LITERAL_I64:
            print_to_builder(string_builder, "%d, %d - \033[38;5;14mNUMERIC_LITERAL_I64\033[0m: \033[38;5;14m%v\033[0m", tokens[i].line, tokens[i].column, tokens[i].text);
            break;
        case Token_Type::NUMERIC_LITERAL_UI64:
            print_to_builder(string_builder, "%d, %d - \033[38;5;14mNUMERIC_LITERAL_UI64\033[0m: \033[38;5;14m%v\033[0m", tokens[i].line, tokens[i].column, tokens[i].text);
            break;
        case Token_Type::NUMERIC_LITERAL_F32:
            print_to_builder(string_builder, "%d, %d - \033[38;5;14mNUMERIC_LITERAL_F32\033[0m: \033[38;5;14m%v\033[0m", tokens[i].line, tokens[i].column, tokens[i].text);
            break;
        case Token_Type::NUMERIC_LITERAL_F64:
            print_to_builder(string_builder, "%d, %d - \033[38;5;14mNUMERIC_LITERAL_F64\033[0m: \033[38;5;14m%v\033[0m", tokens[i].line, tokens[i].column, tokens[i].text);
            break;
        case Token_Type::NUMERIC_LITERAL_REAL:
            print_to_builder(string_builder, "%d, %d - \033[38;5;14mNUMERIC_LITERAL_REAL\033[0m: \033[38;5;14m%v\033[0m", tokens[i].line, tokens[i].column, tokens[i].text);
            break;
        default:
            print_to_builder(string_builder, "%d, %d - Invalid token type!!!", tokens[i].line, tokens[i].column);
            break;
        }
        print_to_builder(string_builder, "\n");
    }

    if (memory::get_array_size(tokens)) {
        print_to_builder(string_builder, "---\n");
    }

    system::print(to_string(string_builder));
}
