#pragma once

#include "string.hpp"

#include <fstd/language/types.hpp>

// string_view are also used for string literals as the memory of a literal is in the binary.
// In f-lang string literals will be serialized in the binary with the exact same struct than string_view.
// Length will be stored at compile time.
// 
// Flamaros - 16 february 2020

namespace fstd {
	namespace language {
		struct string_view
		{
			uint8_t*	ptr = nullptr;
			size_t		length = 0;
		};

		inline void assign(string_view& str, uint8_t* string)
		{
			str.length = string_literal_length(string);
			str.ptr = string;
		}

		inline void assign(string_view& str, uint8_t* start, size_t length)
		{
			str.ptr = start;
			str.length = length;
		}

		inline void assign(string_view& str, const string& string)
		{
			str.ptr = to_utf8(string);
			str.length = get_string_length(string);
		}

		inline void resize(string_view& str, size_t length)
		{
			str.length = length;
		}

		// @TODO should be in string, but C++ (cycle inclusion)
		inline void copy(string& str, size_t position, const string_view& string)
		{
			memory::array_copy(str.buffer, position, string.ptr, string.length);
		}

		inline size_t get_string_length(const string_view& str)
		{
			return str.length;
		}

		// @Warning be careful when using the resulting buffer
		// there is no ending '/0'
		inline uint8_t* to_utf8(const string_view& str)
		{
			return str.ptr;
		}

		inline bool are_equals(const string_view& a, const string_view& b)
		{
			if (a.length != b.length) {
				return false;
			}
			else if (a.ptr == b.ptr) {	// string_view on exact same string
				return true;
			}
			else {
				return system::memory_compare(a.ptr, b.ptr, a.length);
			}
		}

		// =====================================================================

		struct utf16_string_view
		{
			uint16_t*	ptr = nullptr;
			size_t		length = 0;
		};

		inline void assign(utf16_string_view& str, uint16_t* string)
		{
			str.length = string_literal_length(string);
			str.ptr = string;
		}

		inline void assign(utf16_string_view& str, uint16_t* start, size_t length)
		{
			str.ptr = start;
			str.length = length;
		}

		inline void assign(utf16_string_view& str, const utf16_string& string)
		{
			str.ptr = to_utf16(string);
			str.length = get_string_length(string);
		}

		inline void resize(utf16_string_view& str, size_t length)
		{
			str.length = length;
		}

		// @TODO should be in string, but C++ (cycle inclusion)
		inline void copy(utf16_string& str, size_t position, const utf16_string_view& string)
		{
			memory::array_copy(str.buffer, position, string.ptr, string.length);
		}

		inline size_t get_string_length(const utf16_string_view& str)
		{
			return str.length;
		}

		// @Warning be careful when using the resulting buffer
		// there is no ending '/0'
		inline uint16_t* to_utf16(const utf16_string_view& str)
		{
			return str.ptr;
		}

		inline bool are_equals(const utf16_string_view& a, const utf16_string_view& b)
		{
			if (a.length != b.length) {
				return false;
			}
			else if (a.ptr == b.ptr) {	// string_view on exact same string
				return true;
			}
			else {
				return system::memory_compare(a.ptr, b.ptr, a.length);
			}
		}
	}
}
