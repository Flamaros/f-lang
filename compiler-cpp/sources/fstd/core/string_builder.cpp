#include "string_builder.hpp"

#include <fstd/language/intrinsic.hpp>

#include <fstd/core/assert.hpp>

#include <cstdarg>	// @TODO remove it

using namespace fstd::system;

namespace fstd
{
	namespace core
	{
		static void print_to_builder(String_Builder& builder, const uint8_t* string, size_t length)
		{
			if (length == 0) {
				return;
			}

			language::string* string_buffer;

			memory::array_push_back(builder.strings, language::string());
			string_buffer = memory::get_array_last_element(builder.strings);

			language::copy(*string_buffer, 0, string, length);
		}

		static void print_to_builder(String_Builder& builder, int32_t value)
		{
			language::string* string_buffer;

			memory::array_push_back(builder.strings, language::string());
			string_buffer = memory::get_array_last_element(builder.strings);

			*string_buffer = language::to_string(value);
		}

		static void print_to_builder(String_Builder& builder, int32_t value, int32_t base)
		{
			Assert(base >= 2 && base <= 16);

			language::string*	string_buffer;

			memory::array_push_back(builder.strings, language::string());
			string_buffer = memory::get_array_last_element(builder.strings);

			*string_buffer = language::to_string(value, base);
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
				if (language::to_uft8(*format)[position] == '%')
				{
					// Flush the current text that come from the format string
					print_to_builder(builder, &language::to_uft8(*format)[start_print_position], next_print_length);

					position++;
					next_print_length = 0;

					if (language::to_uft8(*format)[position] == '%') {
						// no vararg in this case
						print_to_builder(builder, &language::to_uft8(*format)[position], 1);

						position++;
					}
					else if (language::to_uft8(*format)[position] == 'd') {
						int32_t value = va_arg(args, int32_t);

						print_to_builder(builder, value);
						position++;
					}

					start_print_position = position;
				}
				else {
					position++;
					next_print_length++;
				}
			}

			// Flush the last text chunk if there is one
			if (next_print_length) {
				print_to_builder(builder, &language::to_uft8(*format)[start_print_position], next_print_length);
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
