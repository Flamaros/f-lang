#include "main.hpp"

#include "globals.hpp"

#include <lexer/lexer.hpp>
#include <parser/parser.hpp>
#include <IR_generator.hpp>

#include <fstd/system/timer.hpp>
#include <fstd/system/path.hpp>

#include <fstd/core/assert.hpp>
#include <fstd/core/unicode.hpp>

#include <fstd/language/string.hpp>
#include <fstd/language/defer.hpp>

#include <fstd/container/hash_table.hpp>

#include <fstd/os/windows/console.hpp>

#include <third-party/SpookyV2.h>

// @TODO remove it
#include <vector>
#include <string>
#include <cstdint>
#include <cstdlib>
#include <time.h>       /* time */

#include <tracy/Tracy.hpp>

#include "ASM/ASM.hpp"

void test_integer_to_string_performances()
{
	ZoneScopedNC("test_integer_to_string_performances", 0xf05545);

	std::vector<int32_t>	numbers;
	uint64_t				start_time;
	uint64_t				end_time;
	double					f_implemenation_time;
	double					std_implemenation_time;

	std::srand((uint32_t)time(nullptr));

	{
		ZoneScopedNC("Initialize random numbers", 0xf05545);

		numbers.resize(10'000'000);
		for (auto& number : numbers) {
			number = std::rand();
		}
	}

	{
		ZoneScopedNC("f-lang to_string", 0xf05545);

		fstd::language::string f_string;
		start_time = fstd::system::get_time_in_nanoseconds();
		for (auto& number : numbers) {
			fstd::language::to_string(number, f_string);
		}
		end_time = fstd::system::get_time_in_nanoseconds();
		f_implemenation_time = ((end_time - start_time) / 1'000'000.0);

		release(f_string);
	}

	{
		ZoneScopedNC("stl to_string", 0xf05545);

		// @Warning std::string are not allocated for small size (the definition of small size can vary with compilers and platforms)
		std::string std_string;
		start_time = fstd::system::get_time_in_nanoseconds();
		for (auto& number : numbers) {
			std_string = std::to_string(number);
		}
		end_time = fstd::system::get_time_in_nanoseconds();
		std_implemenation_time = ((end_time - start_time) / 1'000'000.0);
	}

	printf("to_string performances (%llu iterations):\n    f-lang: %0.3lf ms\n    std: %0.3lf ms\n", (uint64_t)numbers.size(), f_implemenation_time, std_implemenation_time);
}

void test_unicode_code_point_convversions()
{
	size_t	peek;

	// UTF8
	uint8_t	utf8_buffer[4];

	fstd::core::Assert(fstd::core::from_utf8(0x24, 0x00, 0x00, 0x00, peek) == 0x24);		// '$'
	fstd::core::Assert(fstd::core::from_utf8(0xC3, 0xA9, 0x00, 0x00, peek) == 0xE9);		// 'é'
	fstd::core::Assert(fstd::core::from_utf8(0xE2, 0x82, 0xAC, 0x00, peek) == 0x20AC);		// '€'
	fstd::core::Assert(fstd::core::from_utf8(0xF0, 0x9D, 0x84, 0x9E, peek) == 0x1D11E);		// '𝄞'
	fstd::core::Assert(fstd::core::from_utf8(0xF0, 0x90, 0x90, 0xB7, peek) == 0x10437);		// '𐐷'
	fstd::core::Assert(fstd::core::from_utf8(0xF0, 0xA0, 0x80, 0x80, peek) == 0x20000);		// '𠀀'
	fstd::core::Assert(fstd::core::from_utf8(0xF0, 0xA4, 0xAD, 0xA2, peek) == 0x24B62);		// '𤭢'

	fstd::core::to_utf8(0x24, utf8_buffer);
	fstd::core::Assert(utf8_buffer[0] == 0x24);	// '$'
	fstd::core::to_utf8(0xE9, utf8_buffer);
	fstd::core::Assert(utf8_buffer[0] == 0xC3);	// 'é'
	fstd::core::Assert(utf8_buffer[1] == 0xA9);	// 'é'
	fstd::core::to_utf8(0x20AC, utf8_buffer);
	fstd::core::Assert(utf8_buffer[0] == 0xE2);	// '€'
	fstd::core::Assert(utf8_buffer[1] == 0x82);	// '€'
	fstd::core::Assert(utf8_buffer[2] == 0xAC);	// '€'
	fstd::core::to_utf8(0x1D11E, utf8_buffer);
	fstd::core::Assert(utf8_buffer[0] == 0xF0);	// '𝄞'
	fstd::core::Assert(utf8_buffer[1] == 0x9D);	// '𝄞'
	fstd::core::Assert(utf8_buffer[2] == 0x84);	// '𝄞'
	fstd::core::Assert(utf8_buffer[3] == 0x9E);	// '𝄞'
	fstd::core::to_utf8(0x10437, utf8_buffer);
	fstd::core::Assert(utf8_buffer[0] == 0xF0);	// '𐐷'
	fstd::core::Assert(utf8_buffer[1] == 0x90);	// '𐐷'
	fstd::core::Assert(utf8_buffer[2] == 0x90);	// '𐐷'
	fstd::core::Assert(utf8_buffer[3] == 0xB7);	// '𐐷'
	fstd::core::to_utf8(0x20000, utf8_buffer);
	fstd::core::Assert(utf8_buffer[0] == 0xF0);	// '𠀀'
	fstd::core::Assert(utf8_buffer[1] == 0xA0);	// '𠀀'
	fstd::core::Assert(utf8_buffer[2] == 0x80);	// '𠀀'
	fstd::core::Assert(utf8_buffer[3] == 0x80);	// '𠀀'
	fstd::core::to_utf8(0x24B62, utf8_buffer);
	fstd::core::Assert(utf8_buffer[0] == 0xF0);	// '𤭢'
	fstd::core::Assert(utf8_buffer[1] == 0xA4);	// '𤭢'
	fstd::core::Assert(utf8_buffer[2] == 0xAD);	// '𤭢'
	fstd::core::Assert(utf8_buffer[3] == 0xA2);	// '𤭢'

	// UTF16
	uint16_t	utf16_buffer[2];

	fstd::core::Assert(fstd::core::from_utf16LE(0x0024, 0x00, peek) == 0x24);		// '$'
	fstd::core::Assert(fstd::core::from_utf16LE(0x00E9, 0x00, peek) == 0xE9);		// 'é'
	fstd::core::Assert(fstd::core::from_utf16LE(0x20AC, 0x00, peek) == 0x20AC);		// '€'
	fstd::core::Assert(fstd::core::from_utf16LE(0xD834, 0xDD1E, peek) == 0x1D11E);	// '𝄞'
	fstd::core::Assert(fstd::core::from_utf16LE(0xD801, 0xDC37, peek) == 0x10437);	// '𐐷'
	fstd::core::Assert(fstd::core::from_utf16LE(0xD840, 0xDC00, peek) == 0x20000);	// '𠀀'
	fstd::core::Assert(fstd::core::from_utf16LE(0xD852, 0xDF62, peek) == 0x24B62);	// '𤭢'

	fstd::core::to_utf16LE(0x24, utf16_buffer);
	fstd::core::Assert(utf16_buffer[0] == 0x0024);	// '$'
	fstd::core::to_utf16LE(0xE9, utf16_buffer);
	fstd::core::Assert(utf16_buffer[0] == 0x00E9);	// 'é'
	fstd::core::to_utf16LE(0x20AC, utf16_buffer);
	fstd::core::Assert(utf16_buffer[0] == 0x20AC);	// '€'
	fstd::core::to_utf16LE(0x1D11E, utf16_buffer);
	fstd::core::Assert(utf16_buffer[0] == 0xD834);	// '𝄞'
	fstd::core::Assert(utf16_buffer[1] == 0xDD1E);	// '𝄞'
	fstd::core::to_utf16LE(0x10437, utf16_buffer);
	fstd::core::Assert(utf16_buffer[0] == 0xD801);	// '𐐷'
	fstd::core::Assert(utf16_buffer[1] == 0xDC37);	// '𐐷'
	fstd::core::to_utf16LE(0x20000, utf16_buffer);
	fstd::core::Assert(utf16_buffer[0] == 0xD840);	// '𠀀'
	fstd::core::Assert(utf16_buffer[1] == 0xDC00);	// '𠀀'
	fstd::core::to_utf16LE(0x24B62, utf16_buffer);
	fstd::core::Assert(utf16_buffer[0] == 0xD852);	// '𤭢'
	fstd::core::Assert(utf16_buffer[1] == 0xDF62);	// '𤭢'
}

void test_unicode_string_convversions()
{
	fstd::language::string			utf8_string;
	fstd::language::UTF16LE_string	utf16_string;

	fstd::language::string			to_utf8_string;
	fstd::language::UTF16LE_string	to_utf16_string;

	defer {
		release(utf8_string);
		release(utf16_string);
		release(to_utf8_string);
		release(to_utf16_string);
	};

	fstd::language::assign(utf8_string, (uint8_t*)u8"a0-A9-#%-éù-€-𝄞-𠀀-£-¤");
	fstd::language::assign(utf16_string, (uint16_t*)u"a0-A9-#%-éù-€-𝄞-𠀀-£-¤");
	// Hexa: 61 30  2D  41 39  2D  23 25  2D  E9  F9   2D  20AC	 2D  1D11E   2D  20000   2D  A3  A4
	// Deci: 97 48  45  65 57  45  35 37  45  233 249  45  8364	 45  119070  45  131072  45  163 164

	fstd::core::from_utf8_to_utf16LE(utf8_string, to_utf16_string, true);
	fstd::core::from_utf16LE_to_utf8(utf16_string, to_utf8_string, true);

	fstd::core::Assert(are_equals(to_utf16_string, utf16_string));
	fstd::core::Assert(are_equals(to_utf8_string, utf8_string));
}

void test_AST_operator_precedence()
{
	using namespace f;

	fstd::container::Array<f::Token<f::Keyword>>	tokens;
	Parsing_Result								parsing_result;
	//	IR											ir;
	int											result = 0;
	fstd::system::Path							path;

	defer{ fstd::system::reset_path(path); };

	fstd::system::from_native(path, (uint8_t*)u8R"(.\tests\operators\precedence.f)");

	initialize_lexer();
	lex(path, tokens);

	parse(tokens, parsing_result);

	// We can test the node type after the cast which is used to reduce the code size.
	AST_Statement_Scope* global_scope = (AST_Statement_Scope*)parsing_result.ast_root;
	fstd::core::Assert(global_scope->ast_type == f::Node_Type::STATEMENT_SCOPE);

	// x: i32 = 5 * 3 + 4;
	//
	//      +
	//    *   4
	//  5  3
	AST_Statement_Variable* x_var = (AST_Statement_Variable*)global_scope->first_child;
	{
		fstd::core::Assert(x_var->ast_type == f::Node_Type::STATEMENT_VARIABLE);

		AST_Binary_Operator* first_op = (AST_Binary_Operator*)x_var->expression;
		fstd::core::Assert(first_op->ast_type == f::Node_Type::BINARY_OPERATOR_ADDITION);

		AST_Binary_Operator* second_op = (AST_Binary_Operator*)first_op->left;
		fstd::core::Assert(second_op->ast_type == f::Node_Type::BINARY_OPERATOR_MULTIPLICATION);

		AST_Literal* second_op_left = (AST_Literal*)second_op->left;
		fstd::core::Assert(second_op_left->ast_type == f::Node_Type::STATEMENT_LITERAL);
		fstd::core::Assert(second_op_left->value.value.integer == 5);

		AST_Literal* second_op_right = (AST_Literal*)second_op->right;
		fstd::core::Assert(second_op_right->ast_type == f::Node_Type::STATEMENT_LITERAL);
		fstd::core::Assert(second_op_right->value.value.integer == 3);

		AST_Literal* first_op_right = (AST_Literal*)first_op->right;
		fstd::core::Assert(first_op_right->ast_type == f::Node_Type::STATEMENT_LITERAL);
		fstd::core::Assert(first_op_right->value.value.integer == 4);
	}

	// y: i32 = 4 + 5 * 3;
	//
	//      +
	//    4   *
	//       5  3
	AST_Statement_Variable* y_var = (AST_Statement_Variable*)x_var->sibling;
	{
		fstd::core::Assert(y_var->ast_type == f::Node_Type::STATEMENT_VARIABLE);

		AST_Binary_Operator* first_op = (AST_Binary_Operator*)y_var->expression;
		fstd::core::Assert(first_op->ast_type == f::Node_Type::BINARY_OPERATOR_ADDITION);

		AST_Literal* first_op_left = (AST_Literal*)first_op->left;
		fstd::core::Assert(first_op_left->ast_type == f::Node_Type::STATEMENT_LITERAL);
		fstd::core::Assert(first_op_left->value.value.integer == 4);

		AST_Binary_Operator* second_op = (AST_Binary_Operator*)first_op->right;
		fstd::core::Assert(second_op->ast_type == f::Node_Type::BINARY_OPERATOR_MULTIPLICATION);

		AST_Literal* second_op_left = (AST_Literal*)second_op->left;
		fstd::core::Assert(second_op_left->ast_type == f::Node_Type::STATEMENT_LITERAL);
		fstd::core::Assert(second_op_left->value.value.integer == 5);

		AST_Literal* second_op_right = (AST_Literal*)second_op->right;
		fstd::core::Assert(second_op_right->ast_type == f::Node_Type::STATEMENT_LITERAL);
		fstd::core::Assert(second_op_right->value.value.integer == 3);
	}

	// z: i32 = 5 * (3 + 4);
	//
	//      *
	//    5   +
	//       3  4
	AST_Statement_Variable* z_var = (AST_Statement_Variable*)y_var->sibling;
	{
		fstd::core::Assert(z_var->ast_type == f::Node_Type::STATEMENT_VARIABLE);

		AST_Binary_Operator* first_op = (AST_Binary_Operator*)z_var->expression;
		fstd::core::Assert(first_op->ast_type == f::Node_Type::BINARY_OPERATOR_MULTIPLICATION);

		AST_Literal* first_op_left = (AST_Literal*)first_op->left;
		fstd::core::Assert(first_op_left->ast_type == f::Node_Type::STATEMENT_LITERAL);
		fstd::core::Assert(first_op_left->value.value.integer == 5);

		AST_Binary_Operator* second_op = (AST_Binary_Operator*)first_op->right;
		fstd::core::Assert(second_op->ast_type == f::Node_Type::BINARY_OPERATOR_ADDITION);

		AST_Literal* second_op_left = (AST_Literal*)second_op->left;
		fstd::core::Assert(second_op_left->ast_type == f::Node_Type::STATEMENT_LITERAL);
		fstd::core::Assert(second_op_left->value.value.integer == 3);

		AST_Literal* second_op_right = (AST_Literal*)second_op->right;
		fstd::core::Assert(second_op_right->ast_type == f::Node_Type::STATEMENT_LITERAL);
		fstd::core::Assert(second_op_right->value.value.integer == 4);
	}
}

void test_hash_table()
{
	fstd::container::Hash_Table<uint16_t, fstd::language::string, void*>	hash_table;
	uint64_t	hash;
	uint16_t	short_hash;

	fstd::container::hash_table_init(hash_table, &fstd::language::are_equals);

	fstd::language::string		string_type;
	fstd::language::string		user_type_with_hash_of_string; // We will use the same hash than for the string_type to create a collision

	defer{
		release(string_type);
		release(user_type_with_hash_of_string);
		fstd::container::hash_table_release(hash_table);
	};

	fstd::language::assign(string_type, (uint8_t*)u8"fstd::language::string");
	fstd::language::assign(user_type_with_hash_of_string, (uint8_t*)u8"User_Type_With_Hash_Of_String");

	hash = SpookyHash::Hash64((const void*)fstd::language::to_utf8(string_type), fstd::language::get_string_size(string_type), 0);
	short_hash = hash & 0xffff;
	void* value;

	value = (void*)0x01;
	fstd::container::hash_table_insert(hash_table, short_hash, string_type, value);
	value = (void*)0x02;
	fstd::container::hash_table_insert(hash_table, short_hash, user_type_with_hash_of_string, value);

	// Check if we can correctly get back both value even with the hash collision
	fstd::core::Assert((void*)*fstd::container::hash_table_get(hash_table, short_hash, string_type) == (void*)0x01);
	fstd::core::Assert((void*)*fstd::container::hash_table_get(hash_table, short_hash, user_type_with_hash_of_string) == (void*)0x02);

	// Test if we iterate the right number of times with iterators
	size_t count = 0;
	auto it = fstd::container::hash_table_begin(hash_table);
	auto it_end = fstd::container::hash_table_end(hash_table);
	for (; !fstd::container::equals<uint16_t, fstd::language::string, void*, 512>(it, it_end); fstd::container::hash_table_next<uint16_t, fstd::language::string, void*, 512>(it))
	{
		count++;
		void** value = fstd::container::hash_table_get<uint16_t, fstd::language::string, void*, 512>(it);
		fstd::core::Assert(*value != nullptr);
	}
	fstd::core::Assert(count == 2);
}

void test_number_to_string()
{
	fstd::language::string f_string;
	fstd::language::string result_string;

	// Decimal
	fstd::language::to_string(5'624'978'429, f_string); // 64 bits
	fstd::language::assign(result_string, (uint8_t*)u8"5624978429");
	fstd::core::Assert(fstd::language::are_equals(f_string, result_string));
	release(f_string);

	fstd::language::to_string(-5'624'978'429, f_string); // 64 bits
	fstd::language::assign(result_string, (uint8_t*)u8"-5624978429");
	fstd::core::Assert(fstd::language::are_equals(f_string, result_string));
	release(f_string);

	fstd::language::to_string(2'147'483'647, f_string); // 32 bits
	fstd::language::assign(result_string, (uint8_t*)u8"2147483647");
	fstd::core::Assert(fstd::language::are_equals(f_string, result_string));
	release(f_string);

	fstd::language::to_string(-2'147'483'647, f_string); // 32 bits
	fstd::language::assign(result_string, (uint8_t*)u8"-2147483647");
	fstd::core::Assert(fstd::language::are_equals(f_string, result_string));
	release(f_string);

	// Hexadecimal
	fstd::language::to_string((uint64_t)5'624'978'429, 16, f_string); // 64 bits
	fstd::language::assign(result_string, (uint8_t*)u8"000000014F465BFD");
	fstd::core::Assert(fstd::language::are_equals(f_string, result_string));
	release(f_string);

	fstd::language::to_string((uint64_t)-5'624'978'429, 16, f_string); // 64 bits
	fstd::language::assign(result_string, (uint8_t*)u8"FFFFFFFEB0B9A403");
	fstd::core::Assert(fstd::language::are_equals(f_string, result_string));
	release(f_string);

	fstd::language::to_string((uint32_t)2'147'483'647, 16, f_string); // 32 bits
	fstd::language::assign(result_string, (uint8_t*)u8"7FFFFFFF");
	fstd::core::Assert(fstd::language::are_equals(f_string, result_string));
	release(f_string);

	fstd::language::to_string((uint32_t)-2'147'483'647, 16, f_string); // 32 bits
	fstd::language::assign(result_string, (uint8_t*)u8"80000001");
	fstd::core::Assert(fstd::language::are_equals(f_string, result_string));
	release(f_string);

	fstd::language::to_string((uint32_t)-1, 16, f_string); // 32 bits
	fstd::language::assign(result_string, (uint8_t*)u8"FFFFFFFF");
	fstd::core::Assert(fstd::language::are_equals(f_string, result_string));
	release(f_string);

	// Tester également avec des nombres signé aux limites
	// Egalement avec des doubles
}

int main(int ac, char** av)
{
	// Begin Initialization ================================================

	fstd::system::allocator_initialize();

#if defined(FSTD_OS_WINDOWS)
	fstd::os::windows::enable_default_console_configuration();
	defer{ fstd::os::windows::close_console(); };
#endif

#if defined(TRACY_ENABLE)
	tracy::SetThreadName("Main thread");
#endif

	initialize_globals();

#if defined(FSTD_DEBUG)
	fstd::core::set_log_level(*globals.logger, fstd::core::Log_Level::verbose);
#endif

	FrameMark;
	// End Initialization ================================================

#if 1
#	if !defined(_DEBUG)
	test_integer_to_string_performances();
#	endif
#endif
	f::ASM::test_x64_encoding();

	test_unicode_code_point_convversions();
	test_unicode_string_convversions();
	test_AST_operator_precedence();
	test_hash_table();
	test_number_to_string();

	FrameMark;

	return 0;
}
