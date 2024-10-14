#pragma once

#include <fstd/language/string.hpp>

#include <fstd/system/stdio.hpp>

#include <fstd/container/array.hpp>

#include <cstdarg>	// @TODO remove it

namespace fstd
{
	namespace core
	{
		struct String_Builder
		{
			container::Array<language::string>	strings;
		};

		enum class Numeric_Format
		{
			decimal,
			hexadecimal
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
		void				print_to_builder(String_Builder& builder, const uint8_t* string, size_t size);
		void				print_to_builder(String_Builder& builder, const uint16_t* string, size_t size);
		void				print_to_builder(String_Builder& builder, int32_t value, Numeric_Format format = Numeric_Format::decimal);
		void				print_to_builder(String_Builder& builder, uint32_t value);
		void				print_to_builder(String_Builder& builder, int64_t value, Numeric_Format format = Numeric_Format::decimal);
		void				print_to_builder(String_Builder& builder, uint64_t value);
		void				print_to_builder(String_Builder& builder, language::string value);
		void				print_to_builder(String_Builder& builder, language::string_view string);
		void				print_to_builder(String_Builder& builder, const char* format, ...); // Assume that format is an utf8 C string
		void				print_to_builder(String_Builder& builder, const fstd::language::string* format, ...);
		void				print_to_builder(String_Builder& builder, const language::string* format, va_list arg);

		void				free_buffers(String_Builder& builder);
		language::string	to_string(String_Builder& builder);
	}
}
