#include "lexer.hpp"

#include "hash_table.hpp"
#include "globals.hpp"

#include <fstd/core/logger.hpp>

#include <fstd/system/file.hpp>

#include <fstd/stream/memory_stream.hpp>

#include <fstd/language/defer.hpp>
#include <fstd/language/flags.hpp>
#include <fstd/language/string.hpp>

#include <fstd/memory/hash_table.hpp>

#undef max
#include <tracy/Tracy.hpp>

using namespace f;

using namespace fstd;
using namespace fstd::core;

static const std::size_t    tokens_length_heuristic = 5;

/// Return a key for the punctuation of 2 characters
constexpr static std::uint16_t punctuation_key_2(const uint8_t* str)
{
    return ((std::uint16_t)str[0] << 8) | (std::uint16_t)str[1];
}

static Hash_Table<std::uint16_t, Punctuation, Punctuation::UNKNOWN> punctuation_table_2 = {
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

static Hash_Table<std::uint8_t, Punctuation, Punctuation::UNKNOWN> punctuation_table_1 = {
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

static bool is_white_punctuation(Punctuation punctuation)
{
    return punctuation >= Punctuation::WHITE_CHARACTER;
}

static Punctuation ending_punctuation(const language::string_view& text, int& punctuation_length)
{
    Punctuation punctuation = Punctuation::UNKNOWN;

    punctuation_length = 2;
    if (language::get_string_length(text) >= 2)
        punctuation = punctuation_table_2[punctuation_key_2(language::get_data(text) + language::get_string_length(text) - 2)];
    if (punctuation != Punctuation::UNKNOWN)
        return punctuation;
    punctuation_length = 1;
    if (language::get_string_length(text) >= 1)
        punctuation = punctuation_table_1[*(language::get_data(text) + language::get_string_length(text) - 1)];
    return punctuation;
}

static fstd::memory::Hash_Table_S<uint16_t, fstd::language::string, Keyword, 65536 / 1024>	keywords_safe;

/*
// @TODO in c++20 put the key as std::string
static std::unordered_map<std::string_view, Keyword> keywords = {
    {"import"sv,				Keyword::IMPORT},
    {"enum"sv,					Keyword::ENUM},
    {"struct"sv,				Keyword::STRUCT},
    {"typedef"sv,				Keyword::TYPEDEF},
    {"inline"sv,				Keyword::INLINE},
    {"static"sv,				Keyword::STATIC},
    {"true"sv,					Keyword::TRUE},
    {"false"sv,					Keyword::FALSE},
	{"nullptr"sv,				Keyword::NULLPTR},
	{"immutable"sv,				Keyword::IMMUTABLE},

    // Control flow
    {"if"sv,					Keyword::IF},
    {"else"sv,					Keyword::ELSE},
    {"do"sv,					Keyword::DO},
    {"while"sv,					Keyword::WHILE},
    {"for"sv,					Keyword::FOR},
    {"foreach"sv,				Keyword::FOREACH},
    {"switch"sv,				Keyword::SWITCH},
    {"case"sv,					Keyword::CASE},
    {"default"sv,				Keyword::DEFAULT},
    {"final"sv,					Keyword::FINAL},
    {"return"sv,				Keyword::RETURN},
    {"exit"sv,					Keyword::EXIT},

    // Reserved for futur usage
    {"public,"sv,				Keyword::PUBLIC},
    {"protected,"sv,			Keyword::PROTECTED},
    {"private,"sv,				Keyword::PRIVATE},

    // Types
    {"bool"sv,					Keyword::BOOL},
    {"i8"sv,					Keyword::I8},
    {"ui8"sv,					Keyword::UI8},
    {"i16"sv,					Keyword::I16},
    {"ui16"sv,					Keyword::UI16},
    {"i32"sv,					Keyword::I32},
    {"ui32"sv,					Keyword::UI32},
    {"i64"sv,					Keyword::I64},
    {"ui64"sv,					Keyword::UI64},
    {"f32"sv,					Keyword::F32},
    {"f64"sv,					Keyword::F64},
    {"string"sv,				Keyword::STRING},

	// Special keywords (interpreted by the lexer)
	{"__FILE__"sv,				Keyword::SPECIAL_FILE},
	{"__FILE_FULL_PATH__"sv,	Keyword::SPECIAL_FULL_PATH_FILE},
	{"__LINE__"sv,				Keyword::SPECIAL_LINE},
	{"__MODULE__"sv,			Keyword::SPECIAL_MODULE},
	{"__EOF__"sv,				Keyword::SPECIAL_EOF},
	{"__VENDOR__"sv,			Keyword::SPECIAL_COMPILER_VENDOR},
	{"__VERSION__"sv,			Keyword::SPECIAL_COMPILER_VERSION},
};*/

static Keyword is_keyword(const std::string_view& text)
{
    Assert(false);
    //const auto& it = keywords.find(text);
    //if (it != keywords.end())
    //    return it->second;
    return Keyword::UNKNOWN;
}

static bool is_digit(char character)
{
	if (character >= '0' && character <= '9') {
		return true;
	}
	return false;
}

enum class State
{
    BOM,
    TOKEN,
};

bool f::lex(const fstd::system::Path& path, fstd::memory::Array<Token>& tokens)
{
	ZoneScopedNC("f::lex", 0x1b5e20);

	system::File	        file;
    stream::Memory_Stream	stream;
    State                   state = State::BOM;
    size_t	                nb_tokens_prediction = 0;
    memory::Array<uint8_t>  file_buffer;
    uint64_t                file_size;
    language::string_view   current_view;
    int					    current_line = 1;
    int					    current_column = 1;
    

	system::open_file(file, path, fstd::system::File::Opening_Flag::READ);
    defer{ system::close_file(file); };

    file_size = system::get_file_size(file);

    file_buffer = system::initiate_get_file_content_asynchronously(file);
    defer{ memory::release(file_buffer); };

    stream::initialize_memory_stream(stream, file_buffer);

    // @Warning
    //
    // We read the file in background asynchronously, so be careful to not relly on file states.
    // Instead prefer use the stream to get correct states.
    //
    // Flamaros - 01 february 2020

    while (stream::is_eof(stream) == false)
    {
        if (state == State::BOM)
        {
            wait_for_availabe_asynchronous_content(file, stream::get_position(stream) + 4);
            bool has_utf8_boom = fstd::stream::is_uft8_bom(stream, true);

            if (has_utf8_boom == false) {
                log(*globals.logger, Log_Level::warning, "[f::lexer] File: '%s' doesn't have UTF8 BOM.", system::to_native(path));
                // @TODO print a user warning about that the utf8 BOM is missing
            }

            size_t	nb_tokens_prediction = file_size / tokens_length_heuristic;

            memory::reserve_array(tokens, nb_tokens_prediction);
            state = State::TOKEN;

            language::assign(current_view, stream::get_pointer(stream), 0);
        }
        else if (state == State::TOKEN)
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

                    // @TODO
                    //
                    // Before starting we have to check if it is not a multiple characters punctuations.
                    //
                    // With some punctions emit a particular kind of token, like for comments and string literals.
                    //
                    // Flamaros - 02 february 2020

                    language::assign(current_view, stream::get_pointer(stream), 0);

                    if (stream::get_remaining_size(stream) >= 2) {
                        punctuation_2 = punctuation_table_2[punctuation_key_2(stream::get_pointer(stream))];
                    }

                    if (punctuation_2 == Punctuation::LINE_COMMENT) {
                        stream::peek(stream);
                    }
                    else if (punctuation_2 == Punctuation::OPEN_BLOCK_COMMENT) {
                        stream::peek(stream);
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
                    }

                    memory::array_push_back(tokens, token);

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

                        // @TODO check if it is a keyword
                        token.type = Token_Type::IDENTIFIER;

                        token.text = current_view;
                        token.value.punctuation = Punctuation::UNKNOWN;
                    }
                    else {
                        break;
                    }
                }

                memory::array_push_back(tokens, token);
            }
        }
    }

	if (nb_tokens_prediction < memory::get_array_size(tokens)) {
		log(*globals.logger, Log_Level::warning, "[lexer] Wrong token number prediction. Predicted :%d - Nb tokens: %d - Nb tokens/byte: %.3f", nb_tokens_prediction, memory::get_array_size(tokens), (float)memory::get_array_size(tokens) / (float)file_size);
	}

	return true;
}
