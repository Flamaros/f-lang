#include "lexer.hpp"

#include "hash_table.hpp"

#include <utilities/exit_scope.hpp>
#include <utilities/flags.hpp>

#include <unordered_map>

using namespace std::literals;	// For string literal suffix (conversion to std::string_view)

using namespace f;

static const std::size_t    tokens_length_heuristic = 4;

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
};

static Keyword is_keyword(const std::string_view& text)
{
    const auto& it = keywords.find(text);
    if (it != keywords.end())
        return it->second;
    return Keyword::UNKNOWN;
}

static bool is_digit(char character)
{
	if (character >= '0' && character <= '9') {
		return true;
	}
	return false;
}

enum class Numeric_Value_Mode
{
	DECIMAL,
	BINARY,
	HEXADECIMAL,
};

enum class Numeric_Value_Flag
{
	START_WITH_DIGIT		= 0x0001,
	IS_INTERGER				= 0x0002,
	HAS_EXPONENT			= 0x0004,
	IS_HEXADECIMAL          = 0x0008,
	IS_BINARY				= 0x0010,
	IS_EXPONENT_NEGATIVE	= 0x0020,

	// @Warning suffixes should be at end for has_suffix method
	UNSIGNED_SUFFIX			= 0x0040,
	LONG_SUFFIX				= 0x0080,
	FLOAT_SUFFIX			= 0x0100,
	DOUBLE_SUFFIX			= 0x0200,
};

// @TODO pack this struct members with flags,...
struct Numeric_Value_Context
{
	Numeric_Value_Mode	mode = Numeric_Value_Mode::DECIMAL;
	Numeric_Value_Flag	flags = Numeric_Value_Flag::IS_INTERGER;
	long double			divider = 10;
	size_t				exponent_pos = 0;
	int					exponent = 0;
	size_t				pos = 0;

	bool	has_suffix() const {
		return (uint32_t)flags >= (uint32_t)Numeric_Value_Flag::UNSIGNED_SUFFIX;
	}
};

// to_numeric_value parse a string_view incrementaly to extract a NUMBER.
// It return true if the entiere string was processed, else false, which means that the
// number ended.
// The token value can be partialy filled depending of the type of the number,
// floating points with exponent have the value of the exponent in the Numeric_Value_Context.
// The context also contains some other usefull flags like suffixes.
bool	to_numeric_value(Numeric_Value_Context& context, std::string_view string, Token& token)
{
	defer { context.pos++; };

	if (utilities::is_flag_set(context.flags, Numeric_Value_Flag::IS_INTERGER))	// Integer
	{
		if (context.mode == Numeric_Value_Mode::DECIMAL
			&& string[context.pos] >= '0' && string[context.pos] <= '9'
			&& context.has_suffix() == false) {
			token.value.unsigned_integer = token.value.unsigned_integer * 10 + (string[context.pos] - '0');

			if (context.pos == 0) {
				utilities::set_flag(context.flags, Numeric_Value_Flag::START_WITH_DIGIT);
			}
		}
		else if (context.mode == Numeric_Value_Mode::BINARY
			&& string[context.pos] >= '0' && string[context.pos] <= '1'
			&& context.has_suffix() == false) {
			token.value.unsigned_integer = token.value.unsigned_integer * 2 + (string[context.pos] - '0');
		}
		else if (context.mode == Numeric_Value_Mode::HEXADECIMAL
			&& string[context.pos] >= '0' && string[context.pos] <= '9'
			&& context.has_suffix() == false) {
			token.value.unsigned_integer = token.value.unsigned_integer * 16 + (string[context.pos] - '0');
		}
		else if (context.mode == Numeric_Value_Mode::HEXADECIMAL
			&& string[context.pos] >= 'a' && string[context.pos] <= 'f'
			&& context.has_suffix() == false) {
			token.value.unsigned_integer = token.value.unsigned_integer * 16 + (string[context.pos] - 'a') + 10;
		}
		else if (context.mode == Numeric_Value_Mode::HEXADECIMAL
			&& string[context.pos] >= 'A' && string[context.pos] <= 'F'
			&& context.has_suffix() == false) {
			token.value.unsigned_integer = token.value.unsigned_integer * 16 + (string[context.pos] - 'A') + 10;
		}
		else if (context.pos == 1 && string[0] == '0' && string[context.pos] == 'b') {
			context.mode = Numeric_Value_Mode::BINARY;
			utilities::set_flag(context.flags, Numeric_Value_Flag::IS_BINARY);
		}
		else if (context.pos == 1 && string[0] == '0' && string[context.pos] == 'x') {
			context.mode = Numeric_Value_Mode::HEXADECIMAL;
			context.divider = 1;
			utilities::set_flag(context.flags, Numeric_Value_Flag::IS_HEXADECIMAL);
		}
		else if (string[context.pos] == '.'
			&& context.has_suffix() == false) {
			token.value.real_max = (long double)token.value.unsigned_integer;	// @Warning this operate a conversion from integer to floating point

			utilities::unset_flag(context.flags, Numeric_Value_Flag::IS_INTERGER);
		}
		else if (((string[context.pos] == 'e' && context.mode == Numeric_Value_Mode::DECIMAL)
			|| (string[context.pos] == 'p' && context.mode == Numeric_Value_Mode::HEXADECIMAL))
			&& utilities::is_flag_set(context.flags, Numeric_Value_Flag::HAS_EXPONENT) == false	// @Warning We don't support number with many exponent characters (it seems to be an ERROR)
			&& context.has_suffix() == false) {

			token.value.real_max = (long double)token.value.unsigned_integer;	// @Warning this operate a conversion from integer to floating point

			utilities::unset_flag(context.flags, Numeric_Value_Flag::IS_INTERGER);
			utilities::set_flag(context.flags, Numeric_Value_Flag::HAS_EXPONENT);
			context.exponent_pos = context.pos;
		}
		else if (string[context.pos] == '_'
			&& context.has_suffix() == false) {
		}
		// Suffixes
		else if (string[context.pos] == 'u'
			&& utilities::is_flag_set(context.flags, Numeric_Value_Flag::UNSIGNED_SUFFIX) == false
			&& utilities::is_flag_set(context.flags, Numeric_Value_Flag::IS_INTERGER) == true) {
			utilities::set_flag(context.flags, Numeric_Value_Flag::UNSIGNED_SUFFIX);
		}
		else if (string[context.pos] == 'L'
			&& utilities::is_flag_set(context.flags, Numeric_Value_Flag::LONG_SUFFIX) == false) {
			utilities::set_flag(context.flags, Numeric_Value_Flag::LONG_SUFFIX);
		}
		else if (string[context.pos] == 'f'
			&& utilities::is_flag_set(context.flags, Numeric_Value_Flag::FLOAT_SUFFIX) == false
			&& utilities::is_flag_set(context.flags, Numeric_Value_Flag::UNSIGNED_SUFFIX) == false
			&& utilities::is_flag_set(context.flags, Numeric_Value_Flag::LONG_SUFFIX) == false) {
			utilities::unset_flag(context.flags, Numeric_Value_Flag::IS_INTERGER);	// @Warning it will not have any side effect on the parsing as it is already finished (we are on a suffix)
			utilities::set_flag(context.flags, Numeric_Value_Flag::FLOAT_SUFFIX);
		}
		else if (string[context.pos] == 'd'
			&& utilities::is_flag_set(context.flags, Numeric_Value_Flag::DOUBLE_SUFFIX) == false
			&& utilities::is_flag_set(context.flags, Numeric_Value_Flag::UNSIGNED_SUFFIX) == false) {
			utilities::unset_flag(context.flags, Numeric_Value_Flag::IS_INTERGER);	// @Warning it will not have any side effect on the parsing as it is already finished (we are on a suffix)
			utilities::set_flag(context.flags, Numeric_Value_Flag::DOUBLE_SUFFIX);
		}
		else {
			return false;
		}
	}
	else {	// Real (start to be real only after the '.' character)
		if (utilities::is_flag_set(context.flags, Numeric_Value_Flag::HAS_EXPONENT)) {
			if (string[context.pos] == '+'
				&& context.pos == context.exponent_pos + 1) {
			}
			else if (string[context.pos] == '-'
				&& context.pos == context.exponent_pos + 1) {
				utilities::set_flag(context.flags, Numeric_Value_Flag::IS_EXPONENT_NEGATIVE);
			}
			else if (string[context.pos] >= '0' && string[context.pos] <= '9') {
				if (context.exponent == 0
					&& utilities::is_flag_set(context.flags, Numeric_Value_Flag::IS_EXPONENT_NEGATIVE)
					&& context.has_suffix() == false) {
					context.exponent = -(string[context.pos] - '0');
				}
				else if (context.exponent == 0
					&& context.has_suffix() == false) {
					context.exponent = (string[context.pos] - '0');
				}
				else if (context.has_suffix() == false) {
					context.exponent = context.exponent * 10 + (string[context.pos] - '0');
				}
				else {
					return false;
				}
			}
			else if (string[context.pos] == '_'
				&& context.has_suffix() == false) {
			}
			// Suffixes
			else if (string[context.pos] == 'L'
				&& utilities::is_flag_set(context.flags, Numeric_Value_Flag::LONG_SUFFIX) == false) {
				utilities::set_flag(context.flags, Numeric_Value_Flag::LONG_SUFFIX);
			}
			else if (string[context.pos] == 'f'
				&& utilities::is_flag_set(context.flags, Numeric_Value_Flag::FLOAT_SUFFIX) == false) {
				utilities::set_flag(context.flags, Numeric_Value_Flag::FLOAT_SUFFIX);
			}
			else if (string[context.pos] == 'd'
				&& utilities::is_flag_set(context.flags, Numeric_Value_Flag::DOUBLE_SUFFIX) == false) {
				utilities::set_flag(context.flags, Numeric_Value_Flag::DOUBLE_SUFFIX);
			}
			else {
				return false;
			}
		}
		else if (context.mode == Numeric_Value_Mode::DECIMAL
			&& string[context.pos] >= '0' && string[context.pos] <= '9'
			&& context.has_suffix() == false) {
			token.value.real_max += (string[context.pos] - '0') / context.divider;
			context.divider *= 10;

			if (context.pos == 0) {
				utilities::set_flag(context.flags, Numeric_Value_Flag::START_WITH_DIGIT);
			}
		}
		else if (context.mode == Numeric_Value_Mode::HEXADECIMAL
			&& string[context.pos] >= '0' && string[context.pos] <= '9'
			&& context.has_suffix() == false) {
			token.value.real_max += (string[context.pos] - '0') / context.divider;
			context.divider *= 16;
		}
		else if (context.mode == Numeric_Value_Mode::HEXADECIMAL
			&& string[context.pos] >= 'a' && string[context.pos] <= 'f'
			&& context.has_suffix() == false) {
			token.value.real_max += (string[context.pos] - 'a' + 10) / context.divider;
			context.divider *= 16;
		}
		else if (context.mode == Numeric_Value_Mode::HEXADECIMAL
			&& string[context.pos] >= 'A' && string[context.pos] <= 'F'
			&& context.has_suffix() == false) {
			token.value.real_max += (string[context.pos] - 'A' + 10) / context.divider;
			context.divider *= 16;
		}
		else if (((string[context.pos] == 'e' && context.mode == Numeric_Value_Mode::DECIMAL)
			|| (string[context.pos] == 'p' && context.mode == Numeric_Value_Mode::HEXADECIMAL))
			&& utilities::is_flag_set(context.flags, Numeric_Value_Flag::HAS_EXPONENT) == false	// @Warning We don't support number with many exponent characters (it seems to be an error)
			&& context.has_suffix() == false) {

			utilities::set_flag(context.flags, Numeric_Value_Flag::HAS_EXPONENT);
			context.exponent_pos = context.pos;
		}
		else if (string[context.pos] == '_'
			&& context.has_suffix() == false) {
		}
		// Suffixes
		else if (string[context.pos] == 'L'
			&& utilities::is_flag_set(context.flags, Numeric_Value_Flag::LONG_SUFFIX) == false) {
			utilities::set_flag(context.flags, Numeric_Value_Flag::LONG_SUFFIX);
		}
		else if (string[context.pos] == 'f'
			&& utilities::is_flag_set(context.flags, Numeric_Value_Flag::FLOAT_SUFFIX) == false) {
			utilities::set_flag(context.flags, Numeric_Value_Flag::FLOAT_SUFFIX);
		}
		else if (string[context.pos] == 'd'
			&& utilities::is_flag_set(context.flags, Numeric_Value_Flag::DOUBLE_SUFFIX) == false) {
			utilities::set_flag(context.flags, Numeric_Value_Flag::DOUBLE_SUFFIX);
		}
		else {
			return false;
		}
	}

	return true;
}

std::string* parse_string(std::string_view string_view) {
	std::string* result = new std::string;
	bool	escaping = false;

	result->reserve(string_view.size());
	for (size_t i = 0; i < string_view.size(); i++) {
		if (escaping) {
			if (string_view[i] == '\'') {
				result->push_back('\'');
			}
			else if (string_view[i] == '"') {
				result->push_back('\"');
			}
			else if (string_view[i] == '?') {
				result->push_back('\?');
			}
			else if (string_view[i] == '\\') {
				result->push_back('\\');
			}
			else if (string_view[i] == '0') {
				result->push_back('\0');	// @TODO do we need to stop the string here?
			}
			else if (string_view[i] == 'a') {
				result->push_back('\a');
			}
			else if (string_view[i] == 'b') {
				result->push_back('\b');
			}
			else if (string_view[i] == 'f') {
				result->push_back('\f');
			}
			else if (string_view[i] == 'n') {
				result->push_back('\n');
			}
			else if (string_view[i] == 'r') {
				result->push_back('\r');
			}
			else if (string_view[i] == 't') {
				result->push_back('\t');
			}
			else if (string_view[i] == 'v') {
				result->push_back('\v');
			}
			// @TODO \x hexa decimal character value (2 digits) \u for 4 hexa digits unicode \U for 8 digits unicode
			else {
				// @TODO throw an error, we are trying to escape a character that no has no need to be escaped
				// So we drop it
			}
			escaping = false;
			continue;
		}
		else if (string_view[i] == '\\') {
			escaping = true;
		}

		if (escaping == false) {
			result->push_back(string_view[i]);
		}
	}

	return result;
}

enum class State
{
	CLASSIC,
	NUMERIC_LITERAL,
	STRING_LITERAL_RAW,
	STRING_LITERAL,
	COMMENT_LINE,
	COMMENT_BLOCK,
};

void f::tokenize(const std::string& buffer, std::vector<Token>& tokens)
{
    tokens.reserve(buffer.length() / tokens_length_heuristic);

	std::string_view	token_string;
	std::string_view	punctuation_string;
	const char*			string_views_buffer = buffer.data();	// @Warning all string views are about this string_view_buffer
    const char*			start_position = buffer.data();
	const char*			current_position = start_position;
    int					current_line = 1;
    int					current_column = 1;
    int					text_column = 1;
	State				state = State::CLASSIC;

    Token				token;
	std::string_view	text;
    Punctuation			punctuation = Punctuation::UNKNOWN;
    int					punctuation_length = 0;
	bool				START_WITH_DIGIT = false;

	Numeric_Value_Context	numeric_value_context;
	Numeric_Value_Context	forward_numeric_value_context;	// @Warning Value will not be valid as the first number character will not be processed, only the result of the parsing is interesting

	auto	start_new_token = [&]() {
		start_position = current_position + 1;
		text_column = current_column + 1; // text_column comes 1 here after a line return
	};

	auto    generate_punctuation_token = [&](std::string_view text, Punctuation punctuation, size_t column) {
		token.type = Token_Type::SYNTAXE_OPERATOR;
		token.text = text;
		token.line = current_line;
		token.column = column;
		token.value.punctuation = punctuation;

		tokens.push_back(token);
	};

	auto    generate_numeric_literal_token = [&](const Numeric_Value_Context& numeric_value_context, std::string_view text, size_t column) {
		token.text = text;
		token.line = current_line;
		token.column = column;

		if (utilities::is_flag_set(numeric_value_context.flags, Numeric_Value_Flag::IS_INTERGER)) {
			token.type = Token_Type::NUMERIC_LITERAL_I32;

			if (utilities::is_flag_set(numeric_value_context.flags, Numeric_Value_Flag::UNSIGNED_SUFFIX)
				&& utilities::is_flag_set(numeric_value_context.flags, Numeric_Value_Flag::LONG_SUFFIX)) {
				token.type = Token_Type::NUMERIC_LITERAL_UI64;
			}
			else if (utilities::is_flag_set(numeric_value_context.flags, Numeric_Value_Flag::LONG_SUFFIX)) {
				token.type = Token_Type::NUMERIC_LITERAL_I64;
			}
			else if (utilities::is_flag_set(numeric_value_context.flags, Numeric_Value_Flag::UNSIGNED_SUFFIX)) {
				token.type = token.value.unsigned_integer > 4'294'967'295 ? Token_Type::NUMERIC_LITERAL_UI64 : Token_Type::NUMERIC_LITERAL_UI32;
			}
			else if (token.value.unsigned_integer > 2'147'483'647) {
				token.type = Token_Type::NUMERIC_LITERAL_I64;
			}
		}
		else {
			token.type = Token_Type::NUMERIC_LITERAL_F64;

			if (utilities::is_flag_set(numeric_value_context.flags, Numeric_Value_Flag::HAS_EXPONENT)) {
				if (utilities::is_flag_set(numeric_value_context.flags, Numeric_Value_Flag::IS_HEXADECIMAL)) {
					token.value.real_max = token.value.real_max * powl(2, numeric_value_context.exponent);
				}
				else {
					token.value.real_max = token.value.real_max * powl(10, numeric_value_context.exponent);
				}
			}

			if (utilities::is_flag_set(numeric_value_context.flags, Numeric_Value_Flag::LONG_SUFFIX)) {
				token.type = Token_Type::NUMERIC_LITERAL_REAL;
			}
			else if (utilities::is_flag_set(numeric_value_context.flags, Numeric_Value_Flag::DOUBLE_SUFFIX)) {
				token.type = Token_Type::NUMERIC_LITERAL_F64;
				token.value.real_64 = token.value.real_max;	// A conversion is necessary
			}
			else if (utilities::is_flag_set(numeric_value_context.flags, Numeric_Value_Flag::FLOAT_SUFFIX)) {
				token.type = Token_Type::NUMERIC_LITERAL_F32;
				token.value.real_32 = (float)token.value.real_max;	// A conversion is necessary
			}
			else {
				token.value.real_64 = token.value.real_max;
			}
		}

		tokens.push_back(token);
	};

	auto    generate_keyword_or_identifier_token = [&](std::string_view text, size_t column) {
		token.value.keyword = is_keyword(text);

		token.type = token.value.keyword != Keyword::UNKNOWN ? Token_Type::KEYWORD : Token_Type::IDENTIFIER;
		token.text = text;
		token.line = current_line;
        token.column = column;

        tokens.push_back(token);
    };

	auto    generate_string_literal_token = [&](std::string_view text, size_t column, bool raw) {
		token.type = raw ? Token_Type::STRING_LITERAL_RAW : Token_Type::STRING_LITERAL;
		token.text = text;
		token.line = current_line;
		token.column = column;

		if (raw == false) {
			token.value.string = parse_string(token.text);
		}

		tokens.push_back(token);
	};

	// Extracting token one by one (based on the punctuation)
    bool    eof = buffer.empty();
    while (eof == false)
    {
		std::string_view	forward_text;
        Punctuation			forward_punctuation = Punctuation::UNKNOWN;
        int					forward_punctuation_length = 0;
		Token				forward_token;	// @Warning will not be necessary valid

		if (current_position - start_position == 0) {
			START_WITH_DIGIT = is_digit(*current_position);

			if (START_WITH_DIGIT) {
				numeric_value_context = Numeric_Value_Context();
				forward_numeric_value_context = Numeric_Value_Context();
				forward_numeric_value_context.pos = 1;

				token.value.unsigned_integer = 0;

				state = State::NUMERIC_LITERAL;
			}
		}

        if (current_position - string_views_buffer + 2 <= (int)buffer.length())
        {
            forward_text = std::string_view(start_position, (current_position - start_position) + 2);
            forward_punctuation = ending_punctuation(forward_text, forward_punctuation_length);
        }

        text = std::string_view(start_position, (current_position - start_position) + 1);
        punctuation = ending_punctuation(text, punctuation_length);

		if (punctuation == Punctuation::NEW_LINE_CHARACTER) {
			current_column = 0; // 0 because the current_position was not incremented yet, and the cursor is virtually still on previous line
		}

		if (current_position - string_views_buffer + 1 >= (int)buffer.length()) {
			eof = true;
		}

		switch (state)
		{
		case State::CLASSIC:
			if (punctuation == Punctuation::SINGLE_QUOTE) {
				state = State::STRING_LITERAL_RAW;
			}
			else if (punctuation == Punctuation::DOUBLE_QUOTE) {
				state = State::STRING_LITERAL;
			}
			else if (punctuation == Punctuation::LINE_COMMENT) {
				state = State::COMMENT_LINE;

				token_string = std::string_view(text.data(), text.length() - punctuation_length);

				if (token_string.length()) {
					generate_keyword_or_identifier_token(token_string, text_column);
				}
			}
			else if (punctuation == Punctuation::OPEN_BLOCK_COMMENT) {
				state = State::COMMENT_BLOCK;

				token_string = std::string_view(text.data(), text.length() - punctuation_length);

				if (token_string.length()) {
					generate_keyword_or_identifier_token(token_string, text_column);
				}
			}
			else if ((punctuation != Punctuation::UNKNOWN || eof)
				&& (forward_punctuation == Punctuation::UNKNOWN
					|| forward_punctuation >= Punctuation::TILDE
					|| punctuation <= forward_punctuation))			// @Warning Mutiple characters ponctuation have a lower enum value, <= to manage correctly cases like "///" for comment line
			{
				token_string = std::string_view(text.data(), text.length() - punctuation_length);
				punctuation_string = std::string_view(text.data() + text.length() - punctuation_length, punctuation_length);

				if (token_string.length()) {
					generate_keyword_or_identifier_token(token_string, text_column);
				}

				if (is_white_punctuation(punctuation) == false) {
					generate_punctuation_token(punctuation_string, punctuation, current_column - punctuation_string.length() + 1);
				}

				START_WITH_DIGIT = false;
				start_new_token();
			}
			break;

		case State::NUMERIC_LITERAL:
		{
			bool	result = true;
			bool	forward_result = true;

			result = to_numeric_value(numeric_value_context, text, token);
			if (forward_text.length()) {
				forward_result = to_numeric_value(forward_numeric_value_context, forward_text, forward_token);
			}

			if (forward_result == false
				|| (result && eof))
			{
				state = State::CLASSIC;

				generate_numeric_literal_token(numeric_value_context, text, text_column);

				START_WITH_DIGIT = false;
				start_new_token();
			}
		}
			break;

		case State::STRING_LITERAL_RAW:
			if (punctuation == Punctuation::SINGLE_QUOTE) {
				state = State::CLASSIC;

				token_string = std::string_view(text.data() + 1, text.length() - 2);

				generate_string_literal_token(token_string, text_column, true);

				start_new_token();
			}
			break;

		case State::STRING_LITERAL:
			if (punctuation == Punctuation::DOUBLE_QUOTE) {
				state = State::CLASSIC;

				token_string = std::string_view(text.data() + 1, text.length() - 2);

				generate_string_literal_token(token_string, text_column, false);

				start_new_token();
			}
			break;

		case State::COMMENT_LINE:
			if (current_column == 0 || eof) {	// We go a new line, so the comment ends now
				state = State::CLASSIC;

				start_new_token();
			}
			break;

		case State::COMMENT_BLOCK:
			if (punctuation == Punctuation::CLOSE_BLOCK_COMMENT) {
				state = State::CLASSIC;

				start_new_token();
			}
			break;
		}

		// TODO manage here case of eof with a state different than classic, it means that there is an error
		// string literal not ended correctly,...

		if (punctuation == Punctuation::NEW_LINE_CHARACTER) {
			current_line++;
		}

        current_column++;
        current_position++;
    }
}
