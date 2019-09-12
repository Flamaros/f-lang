#include "lexer.hpp"

#include "hash_table.hpp"

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
    {punctuation_key_2("//"), Punctuation::line_comment},
    {punctuation_key_2("/*"), Punctuation::open_block_comment},
    {punctuation_key_2("*/"), Punctuation::close_block_comment},
    {punctuation_key_2("->"), Punctuation::arrow},
    {punctuation_key_2("&&"), Punctuation::logical_and},
    {punctuation_key_2("||"), Punctuation::logical_or},
    {punctuation_key_2("::"), Punctuation::double_colon},
    {punctuation_key_2("=="), Punctuation::equality_test},
    {punctuation_key_2("!="), Punctuation::difference_test},
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
    {"import"sv,        Keyword::_import},
    {"enum"sv,          Keyword::_enum},
    {"struct"sv,        Keyword::_struct},
    {"typedef"sv,       Keyword::_typedef},
    {"inline"sv,        Keyword::K_inline},
    {"static"sv,        Keyword::_static},
    {"fn"sv,            Keyword::_fn},
    {"true"sv,          Keyword::_true},
    {"false"sv,         Keyword::_false},

    // Control flow
    {"if"sv,            Keyword::_if},
    {"else"sv,          Keyword::_else},
    {"do"sv,            Keyword::_do},
    {"while"sv,         Keyword::_while},
    {"for"sv,           Keyword::_for},
    {"foreach"sv,       Keyword::_foreach},
    {"switch"sv,        Keyword::_switch},
    {"case"sv,          Keyword::_case},
    {"default"sv,       Keyword::_default},
    {"final"sv,         Keyword::_final},
    {"return"sv,        Keyword::_return},
    {"exit"sv,          Keyword::_exit},

    // Reserved for futur usage
    {"public,"sv,       Keyword::_public},
    {"protected,"sv,    Keyword::_protected},
    {"private,"sv,      Keyword::_private},

    // Types
    {"bool"sv,          Keyword::_bool},
    {"i8"sv,            Keyword::_i8},
    {"ui8"sv,           Keyword::_ui8},
    {"i16"sv,           Keyword::_i16},
    {"ui16"sv,          Keyword::_ui16},
    {"i32"sv,           Keyword::_i32},
    {"ui32"sv,          Keyword::_ui32},
    {"i64"sv,           Keyword::_i64},
    {"ui64"sv,          Keyword::_ui64},
    {"f32"sv,           Keyword::_f32},
    {"f64"sv,           Keyword::_f64},
    {"string"sv,        Keyword::_string}
};

static Keyword is_keyword(const std::string_view& text)
{
    const auto& it = keywords.find(text);
    if (it != keywords.end())
        return it->second;
    return Keyword::_unknown;
}

void f::tokenize(const std::string& buffer, std::vector<Token>& tokens)
{
    tokens.reserve(buffer.length() / tokens_length_heuristic);

	std::string_view	previous_token_text;
	std::string_view	punctuation_text;
	const char*			string_views_buffer = buffer.data();	// @Warning all string views are about this string_view_buffer
    const char*			start_position = buffer.data();
	const char*			current_position = start_position;
    int					current_line = 1;
    int					current_column = 1;
    int					text_column = 1;

    Token				token;
	std::string_view	text;
    Punctuation			punctuation = Punctuation::unknown;
    int					punctuation_length = 0;

    auto    generateToken = [&](std::string_view text, Punctuation punctuation, size_t column) {
        token.line = current_line;
        token.column = column;
        token.text = text;
        token.punctuation = punctuation;
        token.keyword = punctuation == Punctuation::unknown ? is_keyword(text) : Keyword::_unknown;

        tokens.push_back(token);
    };

    // Extracting token one by one (based on the punctuation)
    bool    eof = buffer.empty();
    while (eof == false)
    {
		std::string_view	forward_text;
        Punctuation			forward_punctuation = Punctuation::unknown;
        int					forward_punctuation_length = 0;

        if (current_position - string_views_buffer + 2 <= (int)buffer.length())
        {
            forward_text = std::string_view(start_position, (current_position - start_position) + 2);
            forward_punctuation = ending_punctuation(forward_text, forward_punctuation_length);
        }

        text = std::string_view(start_position, (current_position - start_position) + 1);
        punctuation = ending_punctuation(text, punctuation_length);

        if (punctuation == Punctuation::new_line_character)
            current_column = 0; // 0 because the current_position was not incremented yet, and the cursor is virtually still on previous line

        if (punctuation != Punctuation::unknown
            && (forward_punctuation == Punctuation::unknown
                || forward_punctuation >= Punctuation::tilde
                || punctuation <= forward_punctuation))			// @Warning Mutiple characters ponctuation have a lower enum value, <= to manage correctly cases like "///" for comment line
        {
            previous_token_text = std::string_view(text.data(), text.length() - punctuation_length);
            punctuation_text = std::string_view(text.data() + text.length() - punctuation_length, punctuation_length);

            if (previous_token_text.length())
                generateToken(previous_token_text, Punctuation::unknown, text_column);

            if (is_white_punctuation(punctuation) == false)
                generateToken(punctuation_text, punctuation, current_column - punctuation_text.length() + 1);

            start_position = current_position + 1;
            text_column = current_column + 1; // text_column comes 1 here after a line return
        }

        if (current_position - string_views_buffer + 1 >= (int)buffer.length())
        {
            // Handling the case of the last token of stream
            if (punctuation == Punctuation::unknown)
            {
                previous_token_text = std::string_view(text.data(), text.length());

                if (previous_token_text.length())
                    generateToken(previous_token_text, Punctuation::unknown, text_column);
            }

            eof = true;
        }

        if (punctuation == Punctuation::new_line_character)
            current_line++;

        current_column++;
        current_position++;
    }
}