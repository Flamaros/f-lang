#include "string.hpp"

#include <fstd/language/intrinsic.hpp>

#include <fstd/core/assert.hpp>

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
			uint8_t*	string;
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
			uint8_t*	string;
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
