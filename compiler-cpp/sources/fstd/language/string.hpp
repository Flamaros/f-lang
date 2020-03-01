#pragma once

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

#include <fstd/language/types.hpp>

namespace fstd
{
	namespace language
	{
		inline size_t	string_literal_length(const uint8_t* string)
		{
			size_t	result = 0;

			for (; string[result] != 0; result++) {
			}
			return result;
		}

		inline size_t	string_literal_length(const uint16_t* string)
		{
			size_t	result = 0;

			for (; string[result] != 0; result++) {
			}
			return result;
		}

		// =====================================================================

		struct string
		{
			memory::Array<uint8_t>	buffer;
		};

		inline void assign(string& str, const uint8_t* string)
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

		inline void copy(string& str, size_t position, const uint8_t* string, size_t length)
		{
			memory::array_copy(str.buffer, position, string, length);
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
		//inline const wchar_t* to_utf16(const string& str)
		//{
		//	return memory::get_array_data(str.buffer);
		//}

		inline uint8_t* to_utf8(const string& str)
		{
			return memory::get_array_data(str.buffer);
		}

		inline size_t get_string_length(const string& str)
		{
			return memory::get_array_size(str.buffer);
		}

		// @SpeedUp @CleanUp
		// Generic version with the base should the only version
		// but if the base is a constant the compiler should be
		// smart enough to remove conditions that launch the fastest
		// implementation normally at runtime
		//
		// Flamaros - 06 january 2020
		string to_string(int64_t number);
		string to_string(int32_t number, int8_t base);

		// =====================================================================

		struct utf16_string
		{
			memory::Array<uint16_t>	buffer;
		};

		inline void assign(utf16_string& str, const uint16_t* string)
		{
			memory::resize_array(str.buffer, string_literal_length(string));
			system::memory_copy(memory::get_array_data(str.buffer), string, memory::get_array_bytes_size(str.buffer));
		}

		inline void reserve(utf16_string& str, size_t length)
		{
			memory::reserve_array(str.buffer, length);
		}

		inline void resize(utf16_string& str, size_t length)
		{
			memory::resize_array(str.buffer, length);
		}

		inline void copy(utf16_string& str, size_t position, const uint16_t* string, size_t length)
		{
			memory::array_copy(str.buffer, position, string, length);
		}

		inline void copy(utf16_string& str, size_t position, const utf16_string& string)
		{
			memory::array_copy(str.buffer, position, string.buffer);
		}

		inline void release(utf16_string& str)
		{
			memory::release(str.buffer);
		}

		// @Warning be careful when using the resulting buffer
		// there is no ending '/0'
		//inline const wchar_t* to_utf16(const string& str)
		//{
		//	return memory::get_array_data(str.buffer);
		//}

		inline uint16_t* to_utf16(const utf16_string& str)
		{
			return memory::get_array_data(str.buffer);
		}

		inline size_t get_string_length(const utf16_string& str)
		{
			return memory::get_array_size(str.buffer);
		}

		inline bool are_equals(const utf16_string& a, const utf16_string& b)
		{
			if (a.buffer.size != b.buffer.size) {
				return false;
			}
			else {
				return system::memory_compare(a.buffer.ptr, b.buffer.ptr, a.buffer.size);
			}
		}
	}
}
