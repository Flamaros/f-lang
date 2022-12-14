#include "string_builder.hpp"

#include <fstd/language/intrinsic.hpp>
#include <fstd/language/defer.hpp>

#include <fstd/core/assert.hpp>
#include <fstd/core/unicode.hpp>

#include <cstdarg>	// @TODO remove it
#include <string_view>	// @TODO remove it (used for %Cv format option (for magic_enum print))

using namespace fstd::system;

// @SpeedUp
// Take a look to: https://github.com/nothings/stb/blob/master/stb_sprintf.h

namespace fstd
{
	namespace core
	{
		void print_to_builder(String_Builder& builder, const uint8_t* string, size_t size)
		{
			if (size == 0) {
				return;
			}

			language::string* string_buffer;

			memory::array_push_back(builder.strings, language::string());
			string_buffer = memory::get_array_last_element(builder.strings);

			language::copy(*string_buffer, 0, string, size);
		}

		void print_to_builder(String_Builder& builder, const uint16_t* string, size_t size)
		{
			if (size == 0) {
				return;
			}

			language::string* string_buffer;

			memory::array_push_back(builder.strings, language::string());
			string_buffer = memory::get_array_last_element(builder.strings);

			language::UTF16LE_string_view	string_view;

			language::assign(string_view, const_cast<uint16_t*>(string), size);
			from_utf16LE_to_utf8(string_view, *string_buffer, false);
		}

		void print_to_builder(String_Builder& builder, int32_t value, Numeric_Format format)
		{
			language::string* string_buffer;

			if (format == Numeric_Format::hexadecimal) {
				print_to_builder(builder, "0x", 2);
			}

			memory::array_push_back(builder.strings, language::string());
			string_buffer = memory::get_array_last_element(builder.strings);

			if (format == Numeric_Format::decimal) {
				language::to_string(value, *string_buffer);
			}
			else if (format == Numeric_Format::hexadecimal) {
				//*string_buffer = language::to_string(value, 16);
			}
			else {
				core::Assert(false);
			}
		}

		void print_to_builder(String_Builder& builder, uint32_t value)
		{
			language::string* string_buffer;

			memory::array_push_back(builder.strings, language::string());
			string_buffer = memory::get_array_last_element(builder.strings);

			language::to_string(value, *string_buffer);
		}

		void print_to_builder(String_Builder& builder, int64_t value, Numeric_Format format)
		{
			language::string* string_buffer;

			if (format == Numeric_Format::hexadecimal) {
				print_to_builder(builder, "0x", 2);
			}

			memory::array_push_back(builder.strings, language::string());
			string_buffer = memory::get_array_last_element(builder.strings);

			if (format == Numeric_Format::decimal) {
				language::to_string(value, *string_buffer);
			}
			else if (format == Numeric_Format::hexadecimal) {
				//*string_buffer = language::to_string(value, 16);
			}
			else {
				core::Assert(false);
			}
		}

		void print_to_builder(String_Builder& builder, uint64_t value)
		{
			language::string* string_buffer;

			memory::array_push_back(builder.strings, language::string());
			string_buffer = memory::get_array_last_element(builder.strings);

			language::to_string(value, *string_buffer);
		}

		void print_to_builder(String_Builder& builder, float value)
		{
			core::Assert(false); // Not implemented
		}

		void print_to_builder(String_Builder& builder, double value)
		{
			core::Assert(false); // Not implemented
		}

		void print_to_builder(String_Builder& builder, language::string value)
		{
			language::string* string_buffer;

			memory::array_push_back(builder.strings, language::string());
			string_buffer = memory::get_array_last_element(builder.strings);

			language::copy(*string_buffer, 0, value);
		}

		void print_to_builder(String_Builder& builder, language::string_view value)
		{
			language::string* string_buffer;

			memory::array_push_back(builder.strings, language::string());
			string_buffer = memory::get_array_last_element(builder.strings);

			language::copy(*string_buffer, 0, value);
		}

		void print_to_builder(String_Builder& builder, const char* format, ...)
		{
			va_list args;
			va_start(args, format);

			language::string	format_string;

			language::assign(format_string, (uint8_t*)format);
			defer { language::release(format_string); };

			print_to_builder(builder, &format_string, args);

			va_end(args);
		}

		void print_to_builder(String_Builder& builder, const language::string* format, ...)
		{
			va_list args;
			va_start(args, format);

			print_to_builder(builder, format, args);

			va_end(args);
		}

		void print_to_builder(String_Builder& builder, const language::string* format, va_list args)
		{
			size_t		position = 0;
			size_t		start_print_position = 0;
			size_t		next_print_length = 0;
			size_t		format_length = language::get_string_size(*format);

			while (position < format_length) {
				if (language::to_utf8(*format)[position] == '%')
				{
					// Flush the current text that come from the format string
					print_to_builder(builder, &language::to_utf8(*format)[start_print_position], next_print_length);

					position++;
					next_print_length = 0;

					if (language::to_utf8(*format)[position] == '%') {
						// no vararg in this case
						print_to_builder(builder, &language::to_utf8(*format)[position], 1);

						position++;
					}
					else if (language::to_utf8(*format)[position] == 'x') {	// 32 bits hexadecimal
						int32_t value = va_arg(args, int32_t);

						print_to_builder(builder, value, Numeric_Format::hexadecimal);
						position++;
					}
					else if (language::to_utf8(*format)[position] == 'X') {	// 64 bits hexadecimal
						int64_t value = va_arg(args, int64_t);

						print_to_builder(builder, value, Numeric_Format::hexadecimal);
						position++;
					}
					else if (language::to_utf8(*format)[position] == 'p') {	// pointer size hexadecimal
#if defined FSTD_X86_64
						int64_t value = va_arg(args, int64_t);
#else
						int64_t value = va_arg(args, int32_t);
#endif

						print_to_builder(builder, value, Numeric_Format::hexadecimal);
						position++;
					}
					else if (language::to_utf8(*format)[position] == 'd') {	// int32
						int32_t value = va_arg(args, int32_t);

						print_to_builder(builder, value);
						position++;
					}
					else if (language::to_utf8(*format)[position] == 'u') {	// uint32
						uint32_t value = va_arg(args, uint32_t);

						print_to_builder(builder, value);
						position++;
					}
					else if (language::to_utf8(*format)[position] == 'f') {	// f32
						float value = va_arg(args, float);

						print_to_builder(builder, value);
						position++;
					}
					else if (language::to_utf8(*format)[position] == 's') { // string
						language::string value = va_arg(args, language::string);

						print_to_builder(builder, value);
						position++;
					}
					else if (language::to_utf8(*format)[position] == 'v') { // string_view
						language::string_view value = va_arg(args, language::string_view);

						print_to_builder(builder, value);
						position++;
					}
					else if (language::to_utf8(*format)[position] == 'l') { // long modifier
						position++;

						if (language::to_utf8(*format)[position] == 'd') {	// int64
							int64_t value = va_arg(args, int64_t);

							print_to_builder(builder, value);
							position++;
						}
						else if (language::to_utf8(*format)[position] == 'u') {	// uint64
							uint64_t value = va_arg(args, uint64_t);

							print_to_builder(builder, value);
							position++;
						}
						else if (language::to_utf8(*format)[position] == 'f') {	// f64
							double value = va_arg(args, double);

							print_to_builder(builder, value);
							position++;
						}
					}
					else if (language::to_utf8(*format)[position] == 'C') { // C++ modifier @TODO remove that
						position++;

						// This is only made to ease the print with magic_enum
						if (language::to_utf8(*format)[position] == 'v') {	// std::string_view
							std::string_view std_string_view = va_arg(args, std::string_view);

							print_to_builder(builder, (uint8_t*)std_string_view.data(), std_string_view.length());
							position++;
						}
						else if (language::to_utf8(*format)[position] == 's') {	// c string (char*)
							char* c_string = va_arg(args, char*);

							print_to_builder(builder, (uint8_t*)c_string, language::string_literal_size((uint8_t*)c_string));
							position++;
						}
						else if (language::to_utf8(*format)[position] == 'w') {	// c wstring (wchar*)
							wchar_t* w_string = va_arg(args, wchar_t*);

							print_to_builder(builder, (uint16_t*)w_string, language::string_literal_size((uint16_t*)w_string));
							position++;
						}
						else {
							core::Assert(false);
						}

					}
					// @TODO support print of floats
					else {
						core::Assert(false);
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
				print_to_builder(builder, &language::to_utf8(*format)[start_print_position], next_print_length);
			}
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
				total_length += language::get_string_size(*memory::get_array_element(builder.strings, i));
			}

			language::reserve(result, total_length + 1); // + 1 for ending '\0'
			for (size_t i = 0; i < memory::get_array_size(builder.strings); i++) {
				size_t size = language::get_string_size(*memory::get_array_element(builder.strings, i));

				language::copy(result, position, *memory::get_array_element(builder.strings, i));
				position += size;
			}
			result.buffer[total_length] = '\0';

			return result;
		}
	}
}
