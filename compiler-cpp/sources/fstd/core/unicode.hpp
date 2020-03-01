#pragma once

#include <fstd/language/string.hpp>
#include <fstd/language/string_view.hpp>
#include <fstd/language/types.hpp>

namespace fstd
{
	namespace core
	{
		// This method to the uft conversion from utf16 to utf8
		inline void copy(language::string& str, size_t position, const language::utf16_string_view& string)
		{
			memory::reserve_array(str.buffer, position + string.length * 2);

			size_t utf8_length = 0;
			for (size_t i = 0; i < string.length; i++)
			{
				if (string.ptr[i] <= 0xfff) {
					// TODO
					// TODO
					// TODO
					// TODO
					// TODO
					// TODO
					memory::get_array_data(str.buffer)[position + utf8_length] = string.ptr[i];
					utf8_length++;
				}
				else if (string.ptr[i] <= 0xffff) {

					// TODO
					// TODO
					// TODO
					// TODO
					// TODO
					// TODO
					memory::get_array_data(str.buffer)[position + utf8_length] = string.ptr[i];
					utf8_length++;
				}
			}
			memory::resize_array(str.buffer, position + utf8_length);
		}

		// This method to the uft conversion from utf16 to utf8
		inline void copy(language::string& str, size_t position, const language::utf16_string& string)
		{
			language::utf16_string_view utf16_view;

			language::assign(utf16_view, string);
			copy(str, position, utf16_view);
		}
	}
}
