#pragma once

#include <fstd/language/string.hpp>

#include <fstd/system/stdio.hpp>

#include <fstd/memory/array.hpp>

#include <cstdarg>	// @TODO remove it

namespace fstd
{
	namespace core
	{
		struct String_Builder
		{
			memory::Array<language::string>	strings;
		};

		// @TODO @CleanUp format can't be passed as reference due to the usage of va_start that doesn't support it
		// we will try to do something better in f-lang
		// Getting type safe variadic without templates,...
		//
		// Flamaros - 03 january 2020
		void				print_to_builder(String_Builder& builder, const uint8_t* string, size_t size);
		void				print_to_builder(String_Builder& builder, int32_t value);
		void				print_to_builder(String_Builder& builder, int64_t value);
		void				print_to_builder(String_Builder& builder, language::string value);
		void				print_to_builder(String_Builder& builder, language::string_view string);
		void				print_to_builder(String_Builder& builder, const char* format, ...); // Assume that format an utf8 C string
		void				print_to_builder(String_Builder& builder, const fstd::language::string* format, ...);
		void				print_to_builder(String_Builder& builder, const language::string* format, va_list arg);

		void				free_buffers(String_Builder& builder);
		language::string	to_string(String_Builder& builder);
	}
}
