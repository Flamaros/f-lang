#pragma once

// In f-lang string will be in utf-8 as this is the file format of source, string literals
// will be directly in utf-8.

// As in f-lang we will not use the c runtime library we should not have a lot of functions that
// need the '/0' ending character. Maybe for very few OS functions. So it is not planned to store
// it in our strings.
// If it is really needed, the best strategy is certainly to always add this ending character and cheating
// the about size.
//
// We don't have the length of string as it requires to decode all utf8 points to know what is the number of
// characters, and this is often useless to know that. Almost only unicode conversion functions have to take
// care of it.
// Take a look at http://utf8everywhere.org/ to understand on how it can be difficult to count characters, it
// is better to do nothing than doing it wrong here (Ã can be a A followed by ~,...). A font rendering engine
// will have to handle that, so we just have to provide enough functions for it.
//
// Flamaros - 03 january 2020

#include <fstd/memory/array.hpp>

#include <fstd/language/types.hpp>

namespace fstd
{
	namespace language
	{
		inline size_t	string_literal_size(const uint8_t* utf8_string)
		{
			size_t	result = 0;

			for (; utf8_string[result] != 0; result++) {
			}
			return result;
		}

		inline size_t	string_literal_size(const uint16_t* utf16le_string)
		{
			size_t	result = 0;

			for (; utf16le_string[result] != 0; result++) {
			}
			return result;
		}

		// =====================================================================

		struct string
		{
			memory::Array<uint8_t>	buffer;

			// @TODO @CleanUp
			// I don't really want to put the operator here (directly in the struct)
			inline uint8_t& operator[](size_t index)
			{
				return *memory::get_array_element(buffer, index);
			}
		};

		inline void init(string& _string)
		{
			init(_string.buffer);
		}

		inline void assign(string& str, const uint8_t* string)
		{
			memory::resize_array(str.buffer, string_literal_size(string));
			system::memory_copy(memory::get_array_data(str.buffer), string, memory::get_array_bytes_size(str.buffer));
		}

		inline void reserve(string& str, ssize_t size)
		{
			memory::reserve_array(str.buffer, size);
		}

		inline void resize(string& str, ssize_t size)
		{
			memory::resize_array(str.buffer, size);
		}

		inline void copy(string& str, ssize_t position, const uint8_t* string, ssize_t size)
		{
			memory::array_copy(str.buffer, position, string, size);
		}

		inline void copy(string& str, ssize_t position, const string& string, ssize_t start_pos = 0, ssize_t size = -1)
		{
			memory::array_copy(str.buffer, position, string.buffer, start_pos, size);
		}

		inline void release(string& str)
		{
			memory::release(str.buffer);
		}

		// @Warning be careful when using the resulting buffer
		// there is no ending '/0'
		//inline const wchar_t* to_utf16(const utf8_string& str)
		//{
		//	return memory::get_array_data(str.buffer);
		//}

		inline uint8_t* to_utf8(const string& str)
		{
			return memory::get_array_data(str.buffer);
		}

		inline size_t get_string_size(const string& str)
		{
			return memory::get_array_size(str.buffer);
		}

		inline bool are_equals(const string& a, const string& b)
		{
			if (a.buffer.size != b.buffer.size) {
				return false;
			}
			else {
				return system::memory_compare(a.buffer.ptr, b.buffer.ptr, a.buffer.size);
			}
		}

		inline ssize_t first_of(const string& str, uint8_t c, ssize_t start_pos = 0)
		{
			for (ssize_t i = start_pos; i < memory::get_array_size(str.buffer); i++)
			{
				if (*memory::get_array_element(str.buffer, i) == c)
					return i;
			}

			return -1;
		}

		inline ssize_t last_of(const string& str, uint8_t c, ssize_t start_pos = -1)
		{
			if (start_pos == -1) {
				start_pos = memory::get_array_size(str.buffer) - 1;
			}

			core::Assert(start_pos <= memory::get_array_size(str.buffer) - 1);
			for (ssize_t i = start_pos; i != 0; i--)
			{
				if (*memory::get_array_element(str.buffer, i) == c)
					return i;
			}

			return -1;
		}

		// @SpeedUp @CleanUp
		// Generic version with the base should the only version
		// but if the base is a constant the compiler should be
		// smart enough to remove conditions that launch the fastest
		// implementation normally at runtime
		//
		// Flamaros - 06 january 2020
		template<typename IntegerType>
		void to_string(IntegerType number, string& output);

		template<typename IntegerType>
		void to_string(IntegerType number, int8_t base, string& output);

		// =====================================================================

		struct UTF16LE_string
		{
			memory::Array<uint16_t>	buffer;

			// @TODO @CleanUp
			// I don't really want to put the operator here (directly in the struct)
			inline uint16_t& operator[](size_t index)
			{
				return *memory::get_array_element(buffer, index);
			}
		};

		inline void assign(UTF16LE_string& str, const uint16_t* string)
		{
			memory::resize_array(str.buffer, string_literal_size(string));
			system::memory_copy(memory::get_array_data(str.buffer), string, memory::get_array_bytes_size(str.buffer));
		}

		inline void reserve(UTF16LE_string& str, ssize_t size)
		{
			memory::reserve_array(str.buffer, size);
		}

		inline void resize(UTF16LE_string& str, ssize_t size)
		{
			memory::resize_array(str.buffer, size);
		}

		inline void copy(UTF16LE_string& str, ssize_t position, const uint16_t* string, ssize_t size)
		{
			memory::array_copy(str.buffer, position, string, size);
		}

		inline void copy(UTF16LE_string& str, ssize_t position, const UTF16LE_string& string)
		{
			memory::array_copy(str.buffer, position, string.buffer);
		}

		inline void release(UTF16LE_string& str)
		{
			memory::release(str.buffer);
		}

		// @Warning be careful when using the resulting buffer
		// there is no ending '/0'
		//inline const wchar_t* to_utf16(const utf8_string& str)
		//{
		//	return memory::get_array_data(str.buffer);
		//}

		inline uint16_t* to_utf16(const UTF16LE_string& str)
		{
			return memory::get_array_data(str.buffer);
		}

		inline ssize_t get_string_size(const UTF16LE_string& str)
		{
			return memory::get_array_size(str.buffer);
		}

		inline bool are_equals(const UTF16LE_string& a, const UTF16LE_string& b)
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
