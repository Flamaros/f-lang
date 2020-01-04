#include "string_builder.hpp"

#include <fstd/language/intrinsic.hpp>

#include <cassert>

using namespace fstd::system;

namespace fstd
{
	namespace core
	{
		static const wchar_t ordered_digits[] = LR"(0123456789ABCDEF)";

		static void print_to_builder(String_Builder& builder, const uint16_t* string, size_t length)
		{
			if (length == 0) {
				return;
			}

			language::string* string_buffer;

			memory::array_push_back(builder.strings, language::string());
			string_buffer = memory::get_array_last_element(builder.strings);

			language::copy(*string_buffer, 0, (const wchar_t*)string, length);
		}

		// @TODO @CleanUp, we may want to move this kind of function into string
		// if the user want to do simple thing and don' want to instanciate a String_Builder,...
		static void print_to_builder(String_Builder& builder, int32_t value, int32_t base)
		{
			assert(base >= 2 && base <= 16);

			language::string*	string_buffer;
			wchar_t*			string;
			size_t				string_length = 0;	// doesn't contains the sign
			bool				is_negative = false;

			memory::array_push_back(builder.strings, language::string());
			string_buffer = memory::get_array_last_element(builder.strings);

			// value range is from −2,147,483,647 to +2,147,483,647
			// without decoration we need at most 10 + 1 characters (+1 for the sign)
			language::reserve(*string_buffer, 10 + 1);
			string = (wchar_t*)language::to_uft16(*string_buffer);

			// @TODO May we optimize this code?
			{
				uint32_t	quotien = value;
				uint32_t	reminder;

				if (base == 10 && value < 0) {
					is_negative = true;
					string[0] = '-';
					string++;
					quotien = -value;
				}

				if (value) {	// we can't divide 0
					do
					{
						intrinsic::divide(quotien, base, &quotien, &reminder);
						string[string_length++] = ordered_digits[reminder];
					} while (quotien);

					// Reverse the string
					size_t middle_cursor = string_length / 2;
					for (size_t i = 0; i < middle_cursor; i++) {
						intrinsic::swap((uint16_t*)&string[i], (uint16_t*)&string[string_length - i - 1]);
					}
				}
				else {
					string[0] = '0';
				}

			}

			language::resize(*string_buffer, string_length + is_negative);
		}

		void print_to_builder(String_Builder& builder, const language::string* format, ...)
		{
			size_t		position = 0;
			size_t		start_print_position = 0;
			size_t		next_print_length = 0;
			size_t		format_length = language::get_string_length(*format);

			va_list args;
			va_start(args, format);

			while (position < format_length) {
				if (language::to_uft16(*format)[position] == '%')
				{
					// Flush the current text that come from the format string
					print_to_builder(builder, &language::to_uft16(*format)[start_print_position], next_print_length);

					position++;
					next_print_length = 0;

					if (language::to_uft16(*format)[position] == '%') {
						// no vararg in this case
						print_to_builder(builder, &language::to_uft16(*format)[position], 1);

						position++;
					}
					else if (language::to_uft16(*format)[position] == 'd') {
						int32_t value = va_arg(args, int32_t);

						print_to_builder(builder, value, 10);
						position++;
					}

					start_print_position = position;
				}
				else {
					position++;
					next_print_length++;
				}
			}

			va_end(args);
		}

		void free_buffers(String_Builder& builder)
		{
			for (size_t i = 0; i < memory::get_array_size(builder.strings); i++) {
				language::release(*memory::get_array_element(builder.strings, i));
			}

			memory::release(builder.strings);
		}

		language::string to_string(String_Builder& builder)
		{
			language::string	result;
			size_t				total_length = 0;
			size_t				position = 0;

			for (size_t i = 0; i < memory::get_array_size(builder.strings); i++) {
				total_length += language::get_string_length(*memory::get_array_element(builder.strings, i));
			}

			language::reserve(result, total_length);
			for (size_t i = 0; i < memory::get_array_size(builder.strings); i++) {
				size_t length = language::get_string_length(*memory::get_array_element(builder.strings, i));

				language::copy(result, position, *memory::get_array_element(builder.strings, i));
				position += length;
			}

			return result;
		}
	}
}
