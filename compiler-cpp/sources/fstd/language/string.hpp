#pragma once

// Our string object hold string as UTF16 under Windows and UFT8 on other platforms
// This choice is made to reduce the number of conversions, as under Windows we use
// Wide char APIs in favor of ASCII versions.
// This can be a problem only for compatibility for serialized data, but we will
// provide some conversion functions.


// In f-lang string will be in utf-8 as this is the file format of source, string literals
// will be directly in utf-8
// to build path, there will be a conversion made by the path object when calling win32 APIs



// As in f-lang we will not use the c runtime library we should not have a lot of functions that
// need the '/0' ending character. Maybe for very few OS functions. So it is not planned to store
// it in our strings.
// If it is really needed, the best strategy is certainly to always add this ending character and cheating
// the about length.
//
// Flamaros - 03 january 2020

#include <fstd/memory/array.hpp>

#include <cstdint>
#include <cassert>

namespace fstd
{
	namespace language
	{
		struct immutable_string
		{
			uint16_t*	data = nullptr;
			size_t		length = 0;
		};

		inline size_t	string_literal_length(const wchar_t* string)
		{
			size_t	result = 0;

			for (; string[result] != 0; result++) {
			}
			return result;
		}

		inline void assign(immutable_string& str, const wchar_t* string)
		{
			str.length = string_literal_length(string);
			str.data = (uint16_t*)string;
		}

		// @Warning be careful when using the resulting buffer
		// there is no ending '/0'
		inline uint16_t* to_uft16(const immutable_string& str)
		{
			return str.data;
		}

		inline size_t get_string_length(const immutable_string& str)
		{
			return str.length;
		}

		// =====================================================================

		struct string
		{
			memory::Array<uint16_t>	buffer;
		};

		inline void assign(string& str, const wchar_t* string)
		{
			memory::resize_array(str.buffer, string_literal_length(string));
			system::memory_copy(memory::get_array_data(str.buffer), string, memory::get_array_bytes_size(str.buffer));
		}

		inline void reserve(string& str, size_t length)
		{
			memory::reserve_array(str.buffer, length);
		}

		inline void resize(string& str, size_t length)
		{
			memory::resize_array(str.buffer, length);
		}

		inline void copy(string& str, size_t position, const wchar_t* string, size_t length)
		{
			memory::array_copy(str.buffer, position, (const uint16_t*)string, length);
		}

		inline void copy(string& str, size_t position, const string& string)
		{
			memory::array_copy(str.buffer, position, string.buffer);
		}

		inline void release(string& str)
		{
			memory::release(str.buffer);
		}

		// @Warning be careful when using the resulting buffer
		// there is no ending '/0'
		inline const uint16_t* to_uft16(const string& str)
		{
			return memory::get_array_data(str.buffer);
		}

		inline size_t get_string_length(const string& str)
		{
			return memory::get_array_size(str.buffer);
		}
	}
}
