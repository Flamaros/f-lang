#include "lexer.hpp"

#include "hash_table.hpp"

#include <utilities/exit_scope.hpp>
#include <utilities/flags.hpp>

#include <unordered_map>

using namespace std::literals;	// For string literal suffix (conversion to std::string_view)

using namespace f;

static const std::size_t    tokens_length_heuristic = 6;

/// Return a key for the punctuation of 2 characters
constexpr static std::uint16_t punctuation_key_2(const char* str)
{
    return ((std::uint16_t)str[0] << 8) | (std::uint16_t)str[1];
}

static Hash_Table<std::uint16_t, Punctuation, Punctuation::unknown> punctuation_table_2 = {
    {punctuation_key_2("//"),	Punctuation::line_comment},
    {punctuation_key_2("/*"),	Punctuation::open_block_comment},
    {punctuation_key_2("*/"),	Punctuation::close_block_comment},
    {punctuation_key_2("->"),	Punctuation::arrow},
    {punctuation_key_2("&&"),	Punctuation::logical_and},
    {punctuation_key_2("||"),	Punctuation::logical_or},
    {punctuation_key_2("::"),	Punctuation::double_colon},
	{punctuation_key_2(".."),	Punctuation::double_dot},
	{punctuation_key_2("=="),	Punctuation::equality_test},
    {punctuation_key_2("!="),	Punctuation::difference_test},
	{punctuation_key_2("\\\""),	Punctuation::escaped_double_quote},
};

static Hash_Table<std::uint8_t, Punctuation, Punctuation::unknown> punctuation_table_1 = {
    // White characters (aren't handle for an implicit skip/separation between tokens)
    {' ', Punctuation::white_character},       // space
    {'\t', Punctuation::white_character},      // horizontal tab
    {'\v', Punctuation::white_character},      // vertical tab
    {'\f', Punctuation::white_character},      // feed
    {'\r', Punctuation::white_character},      // carriage return
    {'\n', Punctuation::new_line_character},   // newline
    {'~', Punctuation::tilde},
    {'`', Punctuation::backquote},
    {'!', Punctuation::bang},
    {'@', Punctuation::at},
    {'#', Punctuation::hash},
    {'$', Punctuation::dollar},
    {'%', Punctuation::percent},
    {'^', Punctuation::caret},
    {'&', Punctuation::ampersand},
    {'*', Punctuation::star},
    {'(', Punctuation::open_parenthesis},
    {')', Punctuation::close_parenthesis},
//  {'_', Token::underscore},
    {'-', Punctuation::dash},
    {'+', Punctuation::plus},
    {'=', Punctuation::equals},
    {'{', Punctuation::open_brace},
    {'}', Punctuation::close_brace},
    {'[', Punctuation::open_bracket},
    {']', Punctuation::close_bracket},
    {':', Punctuation::colon},
    {';', Punctuation::semicolon},
    {'\'', Punctuation::single_quote},
    {'"', Punctuation::double_quote},
    {'|', Punctuation::pipe},
    {'/', Punctuation::slash},
    {'\\', Punctuation::backslash},
    {'<', Punctuation::less},
    {'>', Punctuation::greater},
    {',', Punctuation::comma},
    {'.', Punctuation::dot},
    {'?', Punctuation::question_mark}
};

static bool is_white_punctuation(Punctuation punctuation)
{
    return punctuation >= Punctuation::white_character;
}

/// This implemenation doesn't do any lookup in tables
/// Instead it use hash tables specialized by length of punctuation
static Punctuation ending_punctuation(const std::string_view& text, int& punctuation_length)
{
    Punctuation punctuation = Punctuation::unknown;

    punctuation_length = 2;
    if (text.length() >= 2)
        punctuation = punctuation_table_2[punctuation_key_2(text.data() + text.length() - 2)];
    if (punctuation != Punctuation::unknown)
        return punctuation;
    punctuation_length = 1;
    if (text.length() >= 1)
        punctuation = punctuation_table_1[*(text.data() + text.length() - 1)];
    return punctuation;
}

// @TODO in c++20 put the key as std::string
static std::unordered_map<std::string_view, Keyword> keywords = {
    {"import"sv,				Keyword::_import},
    {"enum"sv,					Keyword::_enum},
    {"struct"sv,				Keyword::_struct},
    {"typedef"sv,				Keyword::_typedef},
    {"inline"sv,				Keyword::K_inline},
    {"static"sv,				Keyword::_static},
    {"fn"sv,					Keyword::_fn},
    {"true"sv,					Keyword::_true},
    {"false"sv,					Keyword::_false},
	{"nullptr"sv,				Keyword::_nullptr},
	{"immutable"sv,				Keyword::_immutable},

    // Control flow
    {"if"sv,					Keyword::_if},
    {"else"sv,					Keyword::_else},
    {"do"sv,					Keyword::_do},
    {"while"sv,					Keyword::_while},
    {"for"sv,					Keyword::_for},
    {"foreach"sv,				Keyword::_foreach},
    {"switch"sv,				Keyword::_switch},
    {"case"sv,					Keyword::_case},
    {"default"sv,				Keyword::_default},
    {"final"sv,					Keyword::_final},
    {"return"sv,				Keyword::_return},
    {"exit"sv,					Keyword::_exit},

    // Reserved for futur usage
    {"public,"sv,				Keyword::_public},
    {"protected,"sv,			Keyword::_protected},
    {"private,"sv,				Keyword::_private},

    // Types
    {"bool"sv,					Keyword::_bool},
    {"i8"sv,					Keyword::_i8},
    {"ui8"sv,					Keyword::_ui8},
    {"i16"sv,					Keyword::_i16},
    {"ui16"sv,					Keyword::_ui16},
    {"i32"sv,					Keyword::_i32},
    {"ui32"sv,					Keyword::_ui32},
    {"i64"sv,					Keyword::_i64},
    {"ui64"sv,					Keyword::_ui64},
    {"f32"sv,					Keyword::_f32},
    {"f64"sv,					Keyword::_f64},
    {"string"sv,				Keyword::_string},

	// Special keywords (interpreted by the lexer)
	{"__FILE__"sv,				Keyword::special_file},
	{"__FILE_FULL_PATH__"sv,	Keyword::special_full_path_file},
	{"__LINE__"sv,				Keyword::special_line},
	{"__MODULE__"sv,			Keyword::special_module},
	{"__EOF__"sv,				Keyword::special_eof},
	{"__VENDOR__"sv,			Keyword::special_compiler_vendor},
	{"__VERSION__"sv,			Keyword::special_compiler_version},
};

static Keyword is_keyword(const std::string_view& text)
{
    const auto& it = keywords.find(text);
    if (it != keywords.end())
        return it->second;
    return Keyword::_unknown;
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
	decimal,
	binary,
	hexadecimal,
};

enum class Numeric_Value_Flag
{
	start_with_digit		= 0x0001,
	is_integer				= 0x0002,
	has_exponent			= 0x0004,
	is_hexadecimal          = 0x0008,
	is_binary				= 0x0010,
	is_exponent_negative	= 0x0020,

	// @Warning suffixes should be at end for has_suffix method
	unsigned_suffix			= 0x0040,
	long_suffix				= 0x0080,
	float_suffix			= 0x0100,
	double_suffix			= 0x0200,
};

// @TODO pack this struct members with flags,...
struct Numeric_Value_Context
{
	Numeric_Value_Mode	mode = Numeric_Value_Mode::decimal;
	Numeric_Value_Flag	flags = Numeric_Value_Flag::is_integer;
	long double			divider = 10;
	size_t				exponent_pos = 0;
	int					exponent = 0;
	size_t				pos = 0;

	bool	has_suffix() const {
		return (uint32_t)flags >= (uint32_t)Numeric_Value_Flag::unsigned_suffix;
	}
};

// to_numeric_value parse a string_view incrementaly to extract a number.
// It return true if the entiere string was processed, else false, which means that the
// number ended.
// The token value can be partialy filled depending of the type of the number,
// floating points with exponent have the value of the exponent in the Numeric_Value_Context.
// The context also contains some other usefull flags like suffixes.
bool	to_numeric_value(Numeric_Value_Context& context, std::string_view string, Token& token)
{
	defer { context.pos++; };

	if (utilities::is_flag_set(context.flags, Numeric_Value_Flag::is_integer))	// Integer
	{
		if (context.mode == Numeric_Value_Mode::decimal
			&& string[context.pos] >= '0' && string[context.pos] <= '9'
			&& context.has_suffix() == false) {
			token.value.unsigned_integer = token.value.unsigned_integer * 10 + (string[context.pos] - '0');

			if (context.pos == 0) {
				utilities::set_flag(context.flags, Numeric_Value_Flag::start_with_digit);
			}
		}
		else if (context.mode == Numeric_Value_Mode::binary
			&& string[context.pos] >= '0' && string[context.pos] <= '1'
			&& context.has_suffix() == false) {
			token.value.unsigned_integer = token.value.unsigned_integer * 2 + (string[context.pos] - '0');
		}
		else if (context.mode == Numeric_Value_Mode::hexadecimal
			&& string[context.pos] >= '0' && string[context.pos] <= '9'
			&& context.has_suffix() == false) {
			token.value.unsigned_integer = token.value.unsigned_integer * 16 + (string[context.pos] - '0');
		}
		else if (context.mode == Numeric_Value_Mode::hexadecimal
			&& string[context.pos] >= 'a' && string[context.pos] <= 'f'
			&& context.has_suffix() == false) {
			token.value.unsigned_integer = token.value.unsigned_integer * 16 + (string[context.pos] - 'a') + 10;
		}
		else if (context.mode == Numeric_Value_Mode::hexadecimal
			&& string[context.pos] >= 'A' && string[context.pos] <= 'F'
			&& context.has_suffix() == false) {
			token.value.unsigned_integer = token.value.unsigned_integer * 16 + (string[context.pos] - 'A') + 10;
		}
		else if (context.pos == 1 && string[0] == '0' && string[context.pos] == 'b') {
			context.mode = Numeric_Value_Mode::binary;
			utilities::set_flag(context.flags, Numeric_Value_Flag::is_binary);
		}
		else if (context.pos == 1 && string[0] == '0' && string[context.pos] == 'x') {
			context.mode = Numeric_Value_Mode::hexadecimal;
			context.divider = 1;
			utilities::set_flag(context.flags, Numeric_Value_Flag::is_hexadecimal);
		}
		else if (string[context.pos] == '.'
			&& context.has_suffix() == false) {
			token.value.real_max = (long double)token.value.unsigned_integer;	// @Warning this operate a conversion from integer to floating point

			utilities::unset_flag(context.flags, Numeric_Value_Flag::is_integer);
		}
		else if (((string[context.pos] == 'e' && context.mode == Numeric_Value_Mode::decimal)
			|| (string[context.pos] == 'p' && context.mode == Numeric_Value_Mode::hexadecimal))
			&& utilities::is_flag_set(context.flags, Numeric_Value_Flag::has_exponent) == false	// @Warning We don't support number with many exponent characters (it seems to be an error)
			&& context.has_suffix() == false) {

			token.value.real_max = (long double)token.value.unsigned_integer;	// @Warning this operate a conversion from integer to floating point

			utilities::unset_flag(context.flags, Numeric_Value_Flag::is_integer);
			utilities::set_flag(context.flags, Numeric_Value_Flag::has_exponent);
			context.exponent_pos = context.pos;
		}
		else if (string[context.pos] == '_'
			&& context.has_suffix() == false) {
		}
		// Suffixes
		else if (string[context.pos] == 'u'
			&& utilities::is_flag_set(context.flags, Numeric_Value_Flag::unsigned_suffix) == false
			&& utilities::is_flag_set(context.flags, Numeric_Value_Flag::is_integer) == true) {
			utilities::set_flag(context.flags, Numeric_Value_Flag::unsigned_suffix);
		}
		else if (string[context.pos] == 'L'
			&& utilities::is_flag_set(context.flags, Numeric_Value_Flag::long_suffix) == false) {
			utilities::set_flag(context.flags, Numeric_Value_Flag::long_suffix);
		}
		else if (string[context.pos] == 'f'
			&& utilities::is_flag_set(context.flags, Numeric_Value_Flag::float_suffix) == false
			&& utilities::is_flag_set(context.flags, Numeric_Value_Flag::unsigned_suffix) == false
			&& utilities::is_flag_set(context.flags, Numeric_Value_Flag::long_suffix) == false) {
			utilities::unset_flag(context.flags, Numeric_Value_Flag::is_integer);	// @Warning it will not have any side effect on the parsing as it is already finished (we are on a suffix)
			utilities::set_flag(context.flags, Numeric_Value_Flag::float_suffix);
		}
		else if (string[context.pos] == 'd'
			&& utilities::is_flag_set(context.flags, Numeric_Value_Flag::double_suffix) == false
			&& utilities::is_flag_set(context.flags, Numeric_Value_Flag::unsigned_suffix) == false) {
			utilities::unset_flag(context.flags, Numeric_Value_Flag::is_integer);	// @Warning it will not have any side effect on the parsing as it is already finished (we are on a suffix)
			utilities::set_flag(context.flags, Numeric_Value_Flag::double_suffix);
		}
		else {
			return false;
		}
	}
	else {	// Real (start to be real only after the '.' character)
		if (utilities::is_flag_set(context.flags, Numeric_Value_Flag::has_exponent)) {
			if (string[context.pos] == '+'
				&& context.pos == context.exponent_pos + 1) {
			}
			else if (string[context.pos] == '-'
				&& context.pos == context.exponent_pos + 1) {
				utilities::set_flag(context.flags, Numeric_Value_Flag::is_exponent_negative);
			}
			else if (string[context.pos] >= '0' && string[context.pos] <= '9') {
				if (context.exponent == 0
					&& utilities::is_flag_set(context.flags, Numeric_Value_Flag::is_exponent_negative)
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
				&& utilities::is_flag_set(context.flags, Numeric_Value_Flag::long_suffix) == false) {
				utilities::set_flag(context.flags, Numeric_Value_Flag::long_suffix);
			}
			else if (string[context.pos] == 'f'
				&& utilities::is_flag_set(context.flags, Numeric_Value_Flag::float_suffix) == false) {
				utilities::set_flag(context.flags, Numeric_Value_Flag::float_suffix);
			}
			else if (string[context.pos] == 'd'
				&& utilities::is_flag_set(context.flags, Numeric_Value_Flag::double_suffix) == false) {
				utilities::set_flag(context.flags, Numeric_Value_Flag::double_suffix);
			}
			else {
				return false;
			}
		}
		else if (context.mode == Numeric_Value_Mode::decimal
			&& string[context.pos] >= '0' && string[context.pos] <= '9'
			&& context.has_suffix() == false) {
			token.value.real_max += (string[context.pos] - '0') / context.divider;
			context.divider *= 10;

			if (context.pos == 0) {
				utilities::set_flag(context.flags, Numeric_Value_Flag::start_with_digit);
			}
		}
		else if (context.mode == Numeric_Value_Mode::hexadecimal
			&& string[context.pos] >= '0' && string[context.pos] <= '9'
			&& context.has_suffix() == false) {
			token.value.real_max += (string[context.pos] - '0') / context.divider;
			context.divider *= 16;
		}
		else if (context.mode == Numeric_Value_Mode::hexadecimal
			&& string[context.pos] >= 'a' && string[context.pos] <= 'f'
			&& context.has_suffix() == false) {
			token.value.real_max += (string[context.pos] - 'a' + 10) / context.divider;
			context.divider *= 16;
		}
		else if (context.mode == Numeric_Value_Mode::hexadecimal
			&& string[context.pos] >= 'A' && string[context.pos] <= 'F'
			&& context.has_suffix() == false) {
			token.value.real_max += (string[context.pos] - 'A' + 10) / context.divider;
			context.divider *= 16;
		}
		else if (((string[context.pos] == 'e' && context.mode == Numeric_Value_Mode::decimal)
			|| (string[context.pos] == 'p' && context.mode == Numeric_Value_Mode::hexadecimal))
			&& utilities::is_flag_set(context.flags, Numeric_Value_Flag::has_exponent) == false	// @Warning We don't support number with many exponent characters (it seems to be an error)
			&& context.has_suffix() == false) {

			utilities::set_flag(context.flags, Numeric_Value_Flag::has_exponent);
			context.exponent_pos = context.pos;
		}
		else if (string[context.pos] == '_'
			&& context.has_suffix() == false) {
		}
		// Suffixes
		else if (string[context.pos] == 'L'
			&& utilities::is_flag_set(context.flags, Numeric_Value_Flag::long_suffix) == false) {
			utilities::set_flag(context.flags, Numeric_Value_Flag::long_suffix);
		}
		else if (string[context.pos] == 'f'
			&& utilities::is_flag_set(context.flags, Numeric_Value_Flag::float_suffix) == false) {
			utilities::set_flag(context.flags, Numeric_Value_Flag::float_suffix);
		}
		else if (string[context.pos] == 'd'
			&& utilities::is_flag_set(context.flags, Numeric_Value_Flag::double_suffix) == false) {
			utilities::set_flag(context.flags, Numeric_Value_Flag::double_suffix);
		}
		else {
			return false;
		}
	}

	return true;
}

enum class State
{
	classic,
	numeric_literal,
	string_literal_raw,
	string_literal,
	comment_line,
	comment_block,
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
	State				state = State::classic;

    Token				token;
	std::string_view	text;
    Punctuation			punctuation = Punctuation::unknown;
    int					punctuation_length = 0;
	bool				start_with_digit = false;

	Numeric_Value_Context	numeric_value_context;
	Numeric_Value_Context	forward_numeric_value_context;	// @Warning Value will not be valid as the first number character will not be processed, only the result of the parsing is interesting

	auto	start_new_token = [&]() {
		start_position = current_position + 1;
		text_column = current_column + 1; // text_column comes 1 here after a line return
	};

	auto    generate_punctuation_token = [&](std::string_view text, Punctuation punctuation, size_t column) {
		token.type = Token_Type::syntaxe_operator;
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

		if (utilities::is_flag_set(numeric_value_context.flags, Numeric_Value_Flag::is_integer)) {
			token.type = Token_Type::numeric_literal_i32;

			if (utilities::is_flag_set(numeric_value_context.flags, Numeric_Value_Flag::unsigned_suffix)
				&& utilities::is_flag_set(numeric_value_context.flags, Numeric_Value_Flag::long_suffix)) {
				token.type = Token_Type::numeric_literal_ui64;
			}
			else if (utilities::is_flag_set(numeric_value_context.flags, Numeric_Value_Flag::long_suffix)) {
				token.type = Token_Type::numeric_literal_i64;
			}
			else if (utilities::is_flag_set(numeric_value_context.flags, Numeric_Value_Flag::unsigned_suffix)) {
				token.type = token.value.unsigned_integer > 4'294'967'295 ? Token_Type::numeric_literal_ui64 : Token_Type::numeric_literal_ui32;
			}
			else if (token.value.unsigned_integer > 2'147'483'647) {
				token.type = Token_Type::numeric_literal_i64;
			}
		}
		else {
			token.type = Token_Type::numeric_literal_f64;

			if (utilities::is_flag_set(numeric_value_context.flags, Numeric_Value_Flag::has_exponent)) {
				if (utilities::is_flag_set(numeric_value_context.flags, Numeric_Value_Flag::is_hexadecimal)) {
					token.value.real_max = token.value.real_max * powl(2, numeric_value_context.exponent);
				}
				else {
					token.value.real_max = token.value.real_max * powl(10, numeric_value_context.exponent);
				}
			}

			if (utilities::is_flag_set(numeric_value_context.flags, Numeric_Value_Flag::long_suffix)) {
				token.type = Token_Type::numeric_literal_real;
			}
			else if (utilities::is_flag_set(numeric_value_context.flags, Numeric_Value_Flag::double_suffix)) {
				token.type = Token_Type::numeric_literal_f64;
				token.value.real_64 = token.value.real_max;	// A conversion is necessary
			}
			else if (utilities::is_flag_set(numeric_value_context.flags, Numeric_Value_Flag::float_suffix)) {
				token.type = Token_Type::numeric_literal_f32;
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

		token.type = token.value.keyword != Keyword::_unknown ? Token_Type::keyword : Token_Type::identifier;
		token.text = text;
		token.line = current_line;
        token.column = column;

        tokens.push_back(token);
    };

	auto    generate_string_literal_token = [&](std::string_view text, size_t column, bool raw) {
		token.type = raw ? Token_Type::string_literal_raw : Token_Type::string_literal;
		token.text = text;
		token.line = current_line;
		token.column = column;

		tokens.push_back(token);
	};

	// Extracting token one by one (based on the punctuation)
    bool    eof = buffer.empty();
    while (eof == false)
    {
		std::string_view	forward_text;
        Punctuation			forward_punctuation = Punctuation::unknown;
        int					forward_punctuation_length = 0;
		Token				forward_token;	// @Warning will not be necessary valid

		if (current_position - start_position == 0) {
			start_with_digit = is_digit(*current_position);

			if (start_with_digit) {
				numeric_value_context = Numeric_Value_Context();
				forward_numeric_value_context = Numeric_Value_Context();
				forward_numeric_value_context.pos = 1;

				token.value.unsigned_integer = 0;

				state = State::numeric_literal;
			}
		}

        if (current_position - string_views_buffer + 2 <= (int)buffer.length())
        {
            forward_text = std::string_view(start_position, (current_position - start_position) + 2);
            forward_punctuation = ending_punctuation(forward_text, forward_punctuation_length);
        }

        text = std::string_view(start_position, (current_position - start_position) + 1);
        punctuation = ending_punctuation(text, punctuation_length);

		if (punctuation == Punctuation::new_line_character) {
			current_column = 0; // 0 because the current_position was not incremented yet, and the cursor is virtually still on previous line
		}

		if (current_position - string_views_buffer + 1 >= (int)buffer.length()) {
			eof = true;
		}

		switch (state)
		{
		case State::classic:
			if (punctuation == Punctuation::single_quote) {
				state = State::string_literal_raw;
			}
			else if (punctuation == Punctuation::double_quote) {
				state = State::string_literal;
			}
			else if (punctuation == Punctuation::line_comment) {
				state = State::comment_line;
			}
			else if (punctuation == Punctuation::open_block_comment) {
				state = State::comment_block;
			}
			else if ((punctuation != Punctuation::unknown || eof)
				&& (forward_punctuation == Punctuation::unknown
					|| forward_punctuation >= Punctuation::tilde
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

				start_with_digit = false;
				start_new_token();
			}
			break;

		case State::numeric_literal:
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
				state = State::classic;

				generate_numeric_literal_token(numeric_value_context, text, text_column);

				start_with_digit = false;
				start_new_token();
			}
		}
			break;

		case State::string_literal_raw:
			if (punctuation == Punctuation::single_quote) {
				state = State::classic;

				token_string = std::string_view(text.data() + 1, text.length() - 2);

				generate_string_literal_token(token_string, text_column, true);

				start_new_token();
			}
			break;

		case State::string_literal:
			if (punctuation == Punctuation::double_quote) {
				state = State::classic;

				token_string = std::string_view(text.data() + 1, text.length() - 2);

				generate_string_literal_token(token_string, text_column, false);

				start_new_token();
			}
			break;

		case State::comment_line:
			if (current_column == 0 || eof) {	// We go a new line, so the comment ends now
				state = State::classic;

				start_new_token();
			}
			break;

		case State::comment_block:
			if (punctuation == Punctuation::close_block_comment) {
				state = State::classic;

				start_new_token();
			}
			break;
		}

		// TODO manage here case of eof with a state different than classic, it means that there is an error
		// string literal not ended correctly,...

		if (punctuation == Punctuation::new_line_character) {
			current_line++;
		}

        current_column++;
        current_position++;
    }
}
