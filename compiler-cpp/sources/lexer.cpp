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
constexpr static std::uint16_t punctuation_key_2(const char* str)
{
    return ((std::uint16_t)str[0] << 8) | (std::uint16_t)str[1];
}

static Hash_Table<std::uint16_t, Punctuation, Punctuation::UNKNOWN> punctuation_table_2 = {
    {punctuation_key_2("//"),	Punctuation::LINE_COMMENT},
    {punctuation_key_2("/*"),	Punctuation::OPEN_BLOCK_COMMENT},
    {punctuation_key_2("*/"),	Punctuation::CLOSE_BLOCK_COMMENT},
    {punctuation_key_2("->"),	Punctuation::ARROW},
    {punctuation_key_2("&&"),	Punctuation::LOGICAL_AND},
    {punctuation_key_2("||"),	Punctuation::LOGICAL_OR},
    {punctuation_key_2("::"),	Punctuation::DOUBLE_COLON},
	{punctuation_key_2(".."),	Punctuation::DOUBLE_DOT},
	{punctuation_key_2("=="),	Punctuation::EQUALITY_TEST},
    {punctuation_key_2("!="),	Punctuation::DIFFERENCE_TEST},
	{punctuation_key_2("\\\""),	Punctuation::ESCAPED_DOUBLE_QUOTE},
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

/// This implemenation doesn't do any lookup in tables
/// Instead it use hash tables specialized by length of punctuation
static Punctuation ending_punctuation(const std::string_view& text, int& punctuation_length)
{
    Punctuation punctuation = Punctuation::UNKNOWN;

    punctuation_length = 2;
    if (text.length() >= 2)
        punctuation = punctuation_table_2[punctuation_key_2(text.data() + text.length() - 2)];
    if (punctuation != Punctuation::UNKNOWN)
        return punctuation;
    punctuation_length = 1;
    if (text.length() >= 1)
        punctuation = punctuation_table_1[*(text.data() + text.length() - 1)];
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
    assert(false);
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

bool f::lex(const fstd::system::Path& path, fstd::memory::Array<Token>& tokens)
{
	ZoneScopedN("f::lex");

	system::File	file;

	system::open_file(file, path, fstd::system::File::Opening_Flag::READ);

	memory::Array<uint8_t>	source_file_content = fstd::system::get_file_content(file);

	defer{ release(source_file_content); };

	// @TODO We may want to read the file sequentially instead from only one call
	// to @SpeedUp the lexing, the OS will fill his cache for next read while we are
	// lexing chuncks we already have.
	// This can improve total time by almost all time necessary to read the file (startup latency win)

	stream::Memory_Stream	stream;

	stream::initialize_memory_stream(stream, source_file_content);
	bool has_utf8_boom = fstd::stream::is_uft8_bom(stream, true);

	if (has_utf8_boom == false) {
		log(*globals.logger, Log_Level::warning, "[f::lexer] File: '%s' doesn't have UTF8 BOM.", system::to_native(path));
		// @TODO print a user warning about that the utf8 BOM is missing
	}

	size_t	nb_tokens_prediction = memory::get_array_size(source_file_content) / tokens_length_heuristic;

    memory::reserve_array(tokens, nb_tokens_prediction);

	if (nb_tokens_prediction < memory::get_array_size(tokens)) {
		log(*globals.logger, Log_Level::warning, "[lexer] Wrong token number prediction. Predicted :%d - Nb tokens: %d - Nb tokens/byte: %.3f", nb_tokens_prediction, memory::get_array_size(tokens), (float)memory::get_array_size(tokens) / (float)get_array_size(source_file_content));
	}

	return true;
}
