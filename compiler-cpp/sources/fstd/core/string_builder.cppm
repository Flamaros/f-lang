export module fstd.core.string_builder;

import fstd.language.string;

import fstd.system.stdio;

import fstd.memory.array;

#include <cstdarg>	// @TODO remove it

namespace fstd
{
	namespace core
	{
		export
		struct String_Builder
		{
			memory::Array<language::string>	strings;
		};

		// Format codes
		// %% -> '%' (Escape code of % punctuation token)
		//
		// %d -> int32
		// %s -> language::string
		// %v -> language::sring_view
		// %ld -> int64
		//
		// Temporary for C/C++ compatibility
		// %Cv -> std::string_view (form Magic_Enum)
		// %Cs -> char* (0 terminated) @TODO
		// %Cw -> wchar_t* (0 terminated) @TODO

		//  @TODO Put some asserts when there is formats errors

		// @TODO @CleanUp format can't be passed as reference due to the usage of va_start that doesn't support it
		// we will try to do something better in f-lang
		// Getting type safe variadic without templates,...
		//
		// Flamaros - 03 january 2020
		export void				print_to_builder(String_Builder& builder, const uint8_t* string, size_t size);
		export void				print_to_builder(String_Builder& builder, const uint16_t* string, size_t size);
		export void				print_to_builder(String_Builder& builder, int32_t value);
		export void				print_to_builder(String_Builder& builder, int64_t value);
		export void				print_to_builder(String_Builder& builder, language::string value);
		export void				print_to_builder(String_Builder& builder, language::string_view string);
		export void				print_to_builder(String_Builder& builder, const char* format, ...); // Assume that format is an utf8 C string
		export void				print_to_builder(String_Builder& builder, const fstd::language::string* format, ...);
		export void				print_to_builder(String_Builder& builder, const language::string* format, va_list arg);

		export void				free_buffers(String_Builder& builder);
		export language::string	to_string(String_Builder& builder);
	}
}

module: private;

import fstd.language.intrinsic;
#include <fstd/language/defer.hpp>

import fstd.core.assert;
import fstd.core.unicode;

#include <cstdarg>	// @TODO remove it
#include <string_view>	// @TODO remove it

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

		void print_to_builder(String_Builder& builder, int32_t value)
		{
			language::string* string_buffer;

			memory::array_push_back(builder.strings, language::string());
			string_buffer = memory::get_array_last_element(builder.strings);

			*string_buffer = language::to_string(value);
		}

		void print_to_builder(String_Builder& builder, int64_t value)
		{
			language::string* string_buffer;

			memory::array_push_back(builder.strings, language::string());
			string_buffer = memory::get_array_last_element(builder.strings);

			*string_buffer = language::to_string(value);
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

		static void print_to_builder(String_Builder& builder, int32_t value, int32_t base)
		{
			Assert(base >= 2 && base <= 16);

			language::string* string_buffer;

			memory::array_push_back(builder.strings, language::string());
			string_buffer = memory::get_array_last_element(builder.strings);

			*string_buffer = language::to_string(value, base);
		}

		void print_to_builder(String_Builder& builder, const char* format, ...)
		{
			va_list args;
			va_start(args, format);

			language::string	format_string;

			language::assign(format_string, (uint8_t*)format);
			defer{ language::release(format_string); };

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
					else if (language::to_utf8(*format)[position] == 'd') {	// int32
						int32_t value = va_arg(args, int32_t);

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
					}
					else if (language::to_utf8(*format)[position] == 'C') { // C++ modifier @TODO remove that
						position++;

						// This is only made to ease the print with magic_enum
						if (language::to_utf8(*format)[position] == 'v') {	// std::string_view
							std::string_view std_string_view = va_arg(args, std::string_view);

							print_to_builder(builder, (uint8_t*)std_string_view.data(), std_string_view.length());
							position++;
						}
						else if (language::to_utf8(*format)[position] == 's') {	// std::string_view
							char* c_string = va_arg(args, char*);

							print_to_builder(builder, (uint8_t*)c_string, language::string_literal_size((uint8_t*)c_string));
							position++;
						}
						else if (language::to_utf8(*format)[position] == 'w') {	// std::string_view
							wchar_t* w_string = va_arg(args, wchar_t*);

							print_to_builder(builder, (uint16_t*)w_string, language::string_literal_size((uint16_t*)w_string));
							position++;
						}
						else {
							core::Assert(false);
						}

					}
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

			language::reserve(result, total_length);
			for (size_t i = 0; i < memory::get_array_size(builder.strings); i++) {
				size_t size = language::get_string_size(*memory::get_array_element(builder.strings, i));

				language::copy(result, position, *memory::get_array_element(builder.strings, i));
				position += size;
			}

			return result;
		}
	}
}
