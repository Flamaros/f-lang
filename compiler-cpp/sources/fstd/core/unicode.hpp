#pragma once

#include <fstd/language/string.hpp>
#include <fstd/language/string_view.hpp>
#include <fstd/language/types.hpp>

// @TODO replace by my instinsics
#include <intrin.h>

// Module that contains methods to manage conversion between unicode strings.
// There is some that directly give code points and some other optimized for direct conversions.
//
// https://en.wikipedia.org/wiki/UTF-8
// https://en.wikipedia.org/wiki/UTF-16
//
// Flamaros - 07 march 2020

// @TODO @SpeedUp
// Add benchmarks based on utf8 text like: https://raw.githubusercontent.com/cyb70289/utf8/master/UTF-8-demo.txt
//
// Take a look at branchless algorithms and SIMD
// https://nullprogram.com/blog/2017/10/06/
//
// Flamaros - 07 march 2020

namespace fstd
{
	namespace core
	{
		typedef uint32_t Code_Point;

		// Get 4 utf8 code units and return the code point that correspond to the first utf8 character, some units may not be used.
		// peek parameter is incremented by the number of units used to decode the utf8 character.
		inline Code_Point from_utf8(uint8_t code_unit_1, uint8_t code_unit_2, uint8_t code_unit_3, uint8_t code_unit_4, size_t& peek)
		{
			uint8_t nb_leading_zeros = __lzcnt16((uint8_t)~code_unit_1) - 8; // @TODO does the __lzcnt16 exist for 8 bits, because here we have to substract 8 because of that (the high part of the word if full of zeros)

			if (nb_leading_zeros == 0)
			{
				peek += 1;
				return code_unit_1;
			}
			else if (nb_leading_zeros == 2) // 2 bytes
			{
				peek += 2;
				return ((code_unit_1 ^ 0xC0) << 6) | (code_unit_2 ^ 0x80);
			}
			else if (nb_leading_zeros == 3) // 3 bytes
			{
				peek += 3;
				return ((code_unit_1 ^ 0xE0) << 12) | ((code_unit_2 ^ 0x80) << 6) | (code_unit_3 ^ 0x80);
			}
			else // 4 bytes
			{
				peek += 4;
				return ((code_unit_1 ^ 0xF0) << 18) | ((code_unit_2 ^ 0x80) << 12) | ((code_unit_3 ^ 0x80) << 6) | (code_unit_4 ^ 0x80);
			}
		}

		// Get 2 utf16LE code units and return the code point that correspond to the first utf16 character, second unit may not be used.
		// peek parameter is incremented by the number of units used to decode the utf16LE character.
		inline Code_Point from_utf16LE(uint16_t code_unit_1, uint16_t code_unit_2, size_t& peek)
		{
			if ((code_unit_1 & 0xD800) == 0xD800)	// 4 bytes
			{
				peek += 2;
				return ((code_unit_1 ^ 0xD800) << 10) | (code_unit_2 ^ 0xDC00) | 0x10000;
			}
			else	// 2 bytes
			{
				peek += 1;
				return code_unit_1;
			}
		}

		// Encode a code point to utf16LE, the utf16LE character is directly written in the buffer that have to be big enough for 2 utf16 code units (4bytes).
		// The function return the number of code units written.
		inline size_t to_utf16LE(Code_Point code_point, uint16_t* buffer)
		{
			if (code_point >= 0x10000) {
				code_point = code_point ^ 0x10000;	// We add to substract 0x10000
				buffer[0] = (code_point >> 10) | 0xD800;
				buffer[1] = ((uint16_t)code_point) | 0xDC00;
				return 2;
			}
			else
			{
				buffer[0] = (uint16_t)code_point;
				return 1;
			}
		}

		// The '\0' doesn't count in the string size, but the string buffer is reserved according to.
		inline void from_utf8_to_utf16LE(const language::string_view& utf8_string, language::UTF16LE_string& utf16LE_string, bool null_terminated)
		{
			memory::reserve_array(utf16LE_string.buffer, language::get_string_size(utf8_string) + 1);	// We can't have more code units than in utf8 version

			size_t aligned_size_mod = language::get_string_size(utf8_string) % 4;
			size_t aligned_size = language::get_string_size(utf8_string) - aligned_size_mod;
			size_t code_unit_index = 0;
			size_t result_size = 0;

			// @SpeedUp @TODO do a direct conversion from utf8 to utf16LE and utf16LE to utf8

			while (code_unit_index < aligned_size)
			{
				size_t nb_utf16_code_points_added;
				uint8_t code_unit_1 = utf8_string[code_unit_index + 0];
				uint8_t code_unit_2 = utf8_string[code_unit_index + 1];
				uint8_t code_unit_3 = utf8_string[code_unit_index + 2];
				uint8_t code_unit_4 = utf8_string[code_unit_index + 3];
				uint8_t nb_leading_zeros = __lzcnt16((uint8_t)~code_unit_1) - 8; // @TODO does the __lzcnt16 exist for 8 bits, because here we have to substract 8 because of that (the high part of the word if full of zeros)
				uint16_t* output = (uint16_t*)&utf16LE_string[result_size];

				if (nb_leading_zeros == 0)
				{
					code_unit_index += 1;
					nb_utf16_code_points_added = 1;
					output[0] = code_unit_1;
				}
				else if (nb_leading_zeros == 2) // 2 bytes
				{
					code_unit_index += 2;
					nb_utf16_code_points_added = 1;
					output[0] = ((code_unit_1 ^ 0xC0) << 6) | (code_unit_2 ^ 0x80);
				}
				else if (nb_leading_zeros == 3) // 3 bytes
				{
					code_unit_index += 3;
					nb_utf16_code_points_added = 1;
					output[0] = ((code_unit_1 ^ 0xE0) << 12) | ((code_unit_2 ^ 0x80) << 6) | (code_unit_3 ^ 0x80);
				}
				else // 4 bytes, only case for which the code point is > to 0x10000 (and need 2 utf16 code units)
				{
					code_unit_index += 4;
					nb_utf16_code_points_added = 2;
					Code_Point code_point = ((code_unit_1 ^ 0xF0) << 18) | ((code_unit_2 ^ 0x80) << 12) | ((code_unit_3 ^ 0x80) << 6) | (code_unit_4 ^ 0x80);

					// @SpeedUp
					//
					// I don't really know how to optimize this conversion and even if it is possible.
					// The first utf16 code unit require the decoding of first 3 utf8 code units, so at a moment an uint32_t should be instancied so building the complete
					// code point will certainly be as fast.
					//
					// Flamaros - 06 march 2020
					code_point = code_point ^ 0x10000;	// We add to substract 0x10000
					output[0] = (code_point >> 10) | 0xD800;
					output[1] = ((uint16_t)code_point) | 0xDC00;
				}

				result_size += nb_utf16_code_points_added;
			}

			while (code_unit_index < language::get_string_size(utf8_string))
			{
				Code_Point	code_point;

				if (language::get_string_size(utf8_string) - code_unit_index >= 4) {
					code_point = from_utf8(utf8_string[code_unit_index + 0], utf8_string[code_unit_index + 1], utf8_string[code_unit_index + 2], utf8_string[code_unit_index + 3], code_unit_index);
				}
				else if (language::get_string_size(utf8_string) - code_unit_index >= 3) {
					code_point = from_utf8(utf8_string[code_unit_index + 0], utf8_string[code_unit_index + 1], utf8_string[code_unit_index + 2], 0, code_unit_index);
				}
				else if (language::get_string_size(utf8_string) - code_unit_index >= 2) {
					code_point = from_utf8(utf8_string[code_unit_index + 0], utf8_string[code_unit_index + 1], 0, 0, code_unit_index);
				}
				else if (language::get_string_size(utf8_string) - code_unit_index >= 1) {
					code_point = from_utf8(utf8_string[code_unit_index + 0], 0, 0, 0, code_unit_index);
				}

				size_t added = to_utf16LE(code_point, (uint16_t*)&utf16LE_string[result_size]);
				result_size += added;
			}
			language::resize(utf16LE_string, result_size);

			if (null_terminated) {
				*get_array_element(utf16LE_string.buffer, result_size) = 0;
			}
		}

		inline void from_utf8_to_utf16LE(const language::string& utf8_string, language::UTF16LE_string& utf16LE_string, bool null_terminated)
		{
			language::string_view utf8_view;

			language::assign(utf8_view, utf8_string);
			from_utf8_to_utf16LE(utf8_view, utf16LE_string, null_terminated);
		}
	}
}
