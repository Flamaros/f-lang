export module fstd.language.string;

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

import fstd.memory.array;

import fstd.language.types;
import fstd.system.allocator;

namespace fstd
{
	namespace language
	{
		export inline size_t	string_literal_size(const uint8_t* utf8_string)
		{
			size_t	result = 0;

			for (; utf8_string[result] != 0; result++) {
			}
			return result;
		}

		export inline size_t	string_literal_size(const uint16_t* utf16le_string)
		{
			size_t	result = 0;

			for (; utf16le_string[result] != 0; result++) {
			}
			return result;
		}

		// =====================================================================

		export 
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

		export inline void init(string& _string)
		{
			init(_string.buffer);
		}

		export inline void assign(string& str, const uint8_t* string)
		{
			memory::resize_array(str.buffer, string_literal_size(string));
			system::memory_copy(memory::get_array_data(str.buffer), string, memory::get_array_bytes_size(str.buffer));
		}

		export inline void reserve(string& str, size_t size)
		{
			memory::reserve_array(str.buffer, size);
		}

		export inline void resize(string& str, size_t size)
		{
			memory::resize_array(str.buffer, size);
		}

		export inline void copy(string& str, size_t position, const uint8_t* string, size_t size)
		{
			memory::array_copy(str.buffer, position, string, size);
		}

		export inline void copy(string& str, size_t position, const string& string)
		{
			memory::array_copy(str.buffer, position, string.buffer);
		}

		export inline void release(string& str)
		{
			memory::release(str.buffer);
		}

		// @Warning be careful when using the resulting buffer
		// there is no ending '/0'
		//inline const wchar_t* to_utf16(const utf8_string& str)
		//{
		//	return memory::get_array_data(str.buffer);
		//}

		export inline uint8_t* to_utf8(const string& str)
		{
			return memory::get_array_data(str.buffer);
		}

		export inline size_t get_string_size(const string& str)
		{
			return memory::get_array_size(str.buffer);
		}

		export inline bool are_equals(const string& a, const string& b)
		{
			if (a.buffer.size != b.buffer.size) {
				return false;
			}
			else {
				return system::memory_compare(a.buffer.ptr, b.buffer.ptr, a.buffer.size);
			}
		}

		// @SpeedUp @CleanUp
		// Generic version with the base should the only version
		// but if the base is a constant the compiler should be
		// smart enough to remove conditions that launch the fastest
		// implementation normally at runtime
		//
		// Flamaros - 06 january 2020
		export string to_string(int64_t number);
		export string to_string(int32_t number, int8_t base);

		// =====================================================================

		export
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

		export inline void assign(UTF16LE_string& str, const uint16_t* string)
		{
			memory::resize_array(str.buffer, string_literal_size(string));
			system::memory_copy(memory::get_array_data(str.buffer), string, memory::get_array_bytes_size(str.buffer));
		}

		export inline void reserve(UTF16LE_string& str, size_t size)
		{
			memory::reserve_array(str.buffer, size);
		}

		export inline void resize(UTF16LE_string& str, size_t size)
		{
			memory::resize_array(str.buffer, size);
		}

		export inline void copy(UTF16LE_string& str, size_t position, const uint16_t* string, size_t size)
		{
			memory::array_copy(str.buffer, position, string, size);
		}

		export inline void copy(UTF16LE_string& str, size_t position, const UTF16LE_string& string)
		{
			memory::array_copy(str.buffer, position, string.buffer);
		}

		export inline void release(UTF16LE_string& str)
		{
			memory::release(str.buffer);
		}

		// @Warning be careful when using the resulting buffer
		// there is no ending '/0'
		//inline const wchar_t* to_utf16(const utf8_string& str)
		//{
		//	return memory::get_array_data(str.buffer);
		//}

		export inline uint16_t* to_utf16(const UTF16LE_string& str)
		{
			return memory::get_array_data(str.buffer);
		}

		export inline size_t get_string_size(const UTF16LE_string& str)
		{
			return memory::get_array_size(str.buffer);
		}

		export inline bool are_equals(const UTF16LE_string& a, const UTF16LE_string& b)
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

module: private;

import fstd.language.intrinsic;
import fstd.core.assert;

namespace fstd
{
	namespace language
	{
		static const uint8_t* ordered_digits = (uint8_t*)"0123456789ABCDEF";

		static const uint8_t* base_10_digits = (uint8_t*)
			"0001020304050607080910111213141516171819"
			"2021222324252627282930313233343536373839"
			"4041424344454647484950515253545556575859"
			"6061626364656667686970717273747576777879"
			"8081828384858687888990919293949596979899";

		// Inspiration
		// https://github.com/fmtlib/fmt/blob/master/include/fmt/format.h
		//
		// @SpeedUp:
		// 1. intrinsic::divide implementation in ASM for x86 is slower than the C version
		//
		// Flamaros - 23 january 2020

		string to_string(int64_t number)
		{
			string		result;
			uint8_t* string;
			size_t		string_length = 0;	// doesn't contains the sign
			bool		is_negative = false;

			// value range is from -2,147,483,647 to +2,147,483,647
			// without decoration we need at most 10 + 1 characters (+1 for the sign)
			reserve(result, 10 + 1);
			string = (uint8_t*)to_utf8(result);

			// @TODO May we optimize this code?
			{
				uint64_t	quotien = number;
				uint64_t	reminder;

				if (number < 0) {
					is_negative = true;
					string[0] = '-';
					string++;
					quotien = -number;
				}

				while (quotien >= 100) {
					// Integer division is slow so do it for a group of two digits instead
					// of for every digit. The idea comes from the talk by Alexandrescu
					// "Three Optimization Tips for C++". See speed-test for a comparison.
					intrinsic::divide(quotien, 100, &quotien, &reminder);
					reminder = reminder * 2;
					string[string_length++] = base_10_digits[reminder + 1];
					string[string_length++] = base_10_digits[reminder];
				}
				if (quotien < 10) {
					string[string_length++] = '0' + (uint8_t)quotien;
				}
				else {
					reminder = quotien * 2;
					string[string_length++] = base_10_digits[reminder + 1];
					string[string_length++] = base_10_digits[reminder];
				}

				// Reverse the string
				size_t middle_cursor = string_length / 2;
				for (size_t i = 0; i < middle_cursor; i++) {
					intrinsic::swap((uint8_t*)&string[i], (uint8_t*)&string[string_length - i - 1]);
				}
			}

			resize(result, string_length + is_negative);
			return result;
		}

		string to_string(int32_t number, int8_t base)
		{
			fstd::core::Assert(base >= 2 && base <= 16);

			string		result;
			uint8_t* string;
			size_t		string_length = 0;	// doesn't contains the sign
			bool		is_negative = false;

			// value range is from -2,147,483,647 to +2,147,483,647
			// without decoration we need at most 10 + 1 characters (+1 for the sign)
			reserve(result, 10 + 1);
			string = (uint8_t*)to_utf8(result);

			// @TODO May we optimize this code?
			{
				uint32_t	quotien = number;
				uint32_t	reminder;

				if (base == 10 && number < 0) {
					is_negative = true;
					string[0] = '-';
					string++;
					quotien = -number;
				}

				if (number) {	// we can't divide 0
					do
					{
						intrinsic::divide(quotien, base, &quotien, &reminder);
						string[string_length++] = ordered_digits[reminder];
					} while (quotien);

					// Reverse the string
					size_t middle_cursor = string_length / 2;
					for (size_t i = 0; i < middle_cursor; i++) {
						intrinsic::swap((uint8_t*)&string[i], (uint8_t*)&string[string_length - i - 1]);
					}
				}
				else {
					string[0] = '0';
				}
			}

			resize(result, string_length + is_negative);
			return result;
		}
	}
}
