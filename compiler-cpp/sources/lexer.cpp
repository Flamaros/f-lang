#include "lexer.hpp"

#include "hash_table.hpp"

#include <unordered_map>

#include <stdlib.h>	// For atoi, atoll,...

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

/* Return true if it start with a digit character and a number was extracted, '_' are skipped.
 * It can set unsigned_integer or real_max members of Token::Value type.
 * It also return if the passed string represent an integer or a real number.
 */
bool	to_numeric_value(std::string_view string, Token::Value& value, size_t& pos, bool& is_integer)
{
	enum class Mode
	{
		decimal,
		binary,
		hexadecimal,
	};

	bool	start_with_digit = false;
	double	divider = 10;
	Mode	mode = Mode::decimal;

	is_integer = true;	// @Warning used internally also to determine if we pass the entire part of a real number
	value.unsigned_integer = 0;

	for (pos = 0; pos < string.length(); pos++)
	{
		if (mode == Mode::decimal
			&& string[pos] >= '0' && string[pos] <= '9') {
			if (is_integer) {
				value.unsigned_integer = value.unsigned_integer * 10 + (string[pos] - '0');
			}
			else {
				value.real_64 += (string[pos] - '0') / divider;
				divider *= 10;
			}

			if (pos == 0) {
				start_with_digit = true;
			}
		}
		else if (mode == Mode::binary
			&& string[pos] >= '0' && string[pos] <= '1') {
			value.unsigned_integer = value.unsigned_integer * 2 + (string[pos] - '0');
		}
		else if (mode == Mode::hexadecimal) {
			if (string[pos] >= '0' && string[pos] <= '9') {
				value.unsigned_integer = value.unsigned_integer * 16 + (string[pos] - '0');
			}
			else if (string[pos] >= 'a' && string[pos] <= 'f') {
				value.unsigned_integer = value.unsigned_integer * 16 + (string[pos] - 'a') + 10;
			}
			else if (string[pos] >= 'A' && string[pos] <= 'F') {
				value.unsigned_integer = value.unsigned_integer * 16 + (string[pos] - 'A') + 10;
			}
		}
		else if (pos == 1 && string[pos] == 'b') {
			mode = Mode::binary;
		}
		else if (pos == 1 && string[pos] == 'x') {
			mode = Mode::hexadecimal;
		}
		else if (string[pos] == '.' && is_integer == true) {	// @Warning We don't support number with many dot characters (it seems to be an error)
			is_integer = false;
			value.real_64 = value.unsigned_integer;	// @Warning this operate a conversion from integer to floating point
		}
		else if (string[pos] != '_') {
			return start_with_digit;
		}
	}
	return start_with_digit;
}

enum class State
{
	classic,
	numeric_literal,
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

	auto    generate_numeric_literal_token = [&](std::string_view text, size_t column) {
		size_t	pos;
		bool	is_integer;

		token.type = Token_Type::numeric_literal_i32;
		token.text = text;
		token.line = current_line;
		token.column = column;

		to_numeric_value(text, token.value, pos, is_integer);
		
		if (is_integer) {
			if (token.value.unsigned_integer > 2'147'483'647) {
				token.type = Token_Type::numeric_literal_i64;
			}

			if (pos < text.length()) {
				if (pos + 1 == text.length()
					&& text[pos] == 'u') {
					token.type = token.value.unsigned_integer > 4'294'967'295 ? Token_Type::numeric_literal_ui64 : Token_Type::numeric_literal_ui32;
				}
				else if (pos + 1 == text.length()
					&& text[pos] == 'L') {
					token.type = Token_Type::numeric_literal_i64;
				}
				else if (pos + 2 == text.length()
					&& ((text[pos] == 'u'
						&& text[pos + 1] == 'L')
						|| (text[pos] == 'L'
							&& text[pos + 1] == 'u'))) {
					token.type = Token_Type::numeric_literal_ui64;
				}
				else {
					// TODO lexing issue
					// Unreconized suffix
				}
			}
		}
		else {
			token.type = Token_Type::numeric_literal_f64;
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

	auto    generate_string_literal_token = [&](std::string_view text, size_t column) {
		token.type =Token_Type::string_literal;
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

		if (current_position - start_position == 0) {
			start_with_digit = is_digit(*current_position);

			if (start_with_digit) {
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
			if (punctuation == Punctuation::double_quote) {
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
			if ((forward_punctuation != Punctuation::unknown	// @Warning We need to look to the next token type as numbers stop with there last character
				&& forward_punctuation != Punctuation::dot)
				|| eof)
			{
				state = State::classic;
				start_with_digit = false;

				token_string = std::string_view(text.data(), text.length());

				generate_numeric_literal_token(token_string, text_column);

				start_new_token();
			}
			break;

		case State::string_literal:
			if (state == State::string_literal && punctuation == Punctuation::double_quote) {
				state = State::classic;

				token_string = std::string_view(text.data() + 1, text.length() - 2);

				generate_string_literal_token(token_string, text_column);

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
