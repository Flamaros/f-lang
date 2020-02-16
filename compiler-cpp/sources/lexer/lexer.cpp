#include "lexer.hpp"

#include "hash_table.hpp"
#include "keyword_hash_table.hpp"

#include "../globals.hpp"

#include <fstd/core/logger.hpp>
#include <fstd/core/string_builder.hpp>

#include <fstd/system/file.hpp>

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

constexpr static uint16_t keyword_key(const uint8_t* str)
{
    return ((uint16_t)str[0] << 8) | (uint16_t)str[1];
}

static language::string_view keyword_invalid_key;

static Keyword_Hash_Table<uint16_t, language::string_view, Keyword, &keyword_invalid_key, Keyword::UNKNOWN>  keywords;

static inline Keyword is_keyword(const fstd::language::string_view& text)
{
    return keywords.find(keyword_key(language::to_uft8(text)), text);
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
        keywords.insert(keyword_key((uint8_t*)(KEY)), str_view, (Keyword::VALUE)); \
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

        // Special keywords (interpreted by the lexer)
    INSERT_KEYWORD("__FILE__", SPECIAL_FILE);
    INSERT_KEYWORD("__FILE_FULL_PATH__",  SPECIAL_FULL_PATH_FILE);
    INSERT_KEYWORD("__LINE__", SPECIAL_LINE);
    INSERT_KEYWORD("__MODULE__", SPECIAL_MODULE);
    INSERT_KEYWORD("__EOF__", SPECIAL_EOF);
    INSERT_KEYWORD("__VENDOR__", SPECIAL_COMPILER_VENDOR);
    INSERT_KEYWORD("__VERSION__", SPECIAL_COMPILER_VERSION);

    // == Log

	core::String_Builder	string_builder;
	language::string		format;
	language::string		formatted_string;

	defer{
		core::free_buffers(string_builder);
		language::release(formatted_string);
	};

	language::assign(format, (uint8_t*)"[lexer] keywords hash table: nb_used_buckets: %d - nb_collisions: %d\n");
	core::print_to_builder(string_builder, &format, keywords.nb_used_buckets(), keywords.nb_collisions());

	formatted_string = core::to_string(string_builder);
	system::print(formatted_string);
}

bool f::lex(const fstd::system::Path& path, fstd::memory::Array<Token>& tokens)
{
	ZoneScopedNC("f::lex",  0x1b5e20);

	system::File	        file;
    stream::Memory_Stream	stream;
    size_t	                nb_tokens_prediction = 0;
    memory::Array<uint8_t>  file_buffer;
    uint64_t                file_size;
    language::string_view   current_view;
    int					    current_line = 1;
    int					    current_column = 1;

	system::open_file(file, path, fstd::system::File::Opening_Flag::READ);
    // @TODO handle open_file erros

    defer{ system::close_file(file); };

    file_size = system::get_file_size(file);

    file_buffer = system::initiate_get_file_content_asynchronously(file);
    defer{ memory::release(file_buffer); };

    stream::initialize_memory_stream(stream, file_buffer);

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
    bool has_utf8_boom = fstd::stream::is_uft8_bom(stream, true);

    if (has_utf8_boom == false) {
        log(*globals.logger, Log_Level::warning, "[f::lexer] File: '%s' doesn't have UTF8 BOM.",  system::to_native(path));
        // @TODO print a user warning about that the utf8 BOM is missing
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

                stream::peek(stream);
                current_column++;
            }
            else {
                Token       token;
                Punctuation punctuation_2 = Punctuation::UNKNOWN;

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
                        stream::peek(stream);   // @Warning We don't peek the '\n' character (it will be peeked later for the line count increment)
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
                            stream::skip(stream, 2);
                            comment_block_closed = true;
                            break;
                        }

                        stream::peek(stream);
                    }

                    if (comment_block_closed == false) {
                        // @TODO throw a user error
                    }
                }
                else if (punctuation == Punctuation::DOUBLE_QUOTE) {
                    stream::peek(stream);
                }
                else if (punctuation == Punctuation::SINGLE_QUOTE) {
                    stream::peek(stream);
                    //while (stream::is_eof(stream) == false)
                    //{
                    //    uint8_t     current_character;

                    //    current_character = stream::get(stream);
                    //}
                }
                else if (punctuation == Punctuation::BACKQUOTE) {
                    stream::peek(stream);
                }
                else {
                    token.type = Token_Type::SYNTAXE_OPERATOR;

                    if (punctuation_2 != Punctuation::UNKNOWN) {
                        language::assign(current_view, stream::get_pointer(stream), 2);
                        token.text = current_view;
                        token.value.punctuation = punctuation_2;
                        stream::skip(stream, 2);
                    }
                    else {
                        language::assign(current_view, stream::get_pointer(stream), 1);
                        token.text = current_view;
                        token.value.punctuation = punctuation;
                        stream::skip(stream, 1);
                    }

                    memory::array_push_back(tokens, token);
                }

                current_column++;
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

            token.line = current_line;
            token.column = current_column;
                    
            language::assign(current_view, stream::get_pointer(stream), 0);
            while (stream::is_eof(stream) == false)
            {
                uint8_t     current_character;

                current_character = stream::get(stream);

                punctuation = punctuation_table_1[current_character];
                if (punctuation == Punctuation::UNKNOWN) {  // @Warning any kind of punctuation stop the definition of an identifier
                    stream::peek(stream);
                    language::resize(current_view, language::get_string_length(current_view) + 1);
                    current_column++;

                    token.type = Token_Type::IDENTIFIER;

                    token.text = current_view;
                }
                else {
                    break;
                }
            }

            token.value.keyword = is_keyword(token.text);
            memory::array_push_back(tokens, token);
        }
    }

	if (nb_tokens_prediction < memory::get_array_size(tokens)) {
		log(*globals.logger, Log_Level::warning, "[lexer] Wrong token number prediction. Predicted :%d - Nb tokens: %d - Nb tokens/byte: %.3f",  nb_tokens_prediction, memory::get_array_size(tokens), (float)memory::get_array_size(tokens) / (float)file_size);
	}

	return true;
}
