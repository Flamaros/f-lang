#pragma once

#include <fstd/language/string.hpp>

#include <fstd/system/stdio.hpp>

#include <fstd/memory/array.hpp>

#include <cstdarg>

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
		void				print_to_builder(String_Builder& builder, const fstd::language::string* format, ...);

		void				free_buffers(String_Builder& builder);
		language::string	to_string(String_Builder& builder);
	}
}
