#include "string.hpp"

#include <fstd/language/intrinsic.hpp>

namespace fstd
{
	namespace language
	{
		static const wchar_t* ordered_digits = LR"(0123456789ABCDEF)";

		// @TODO @SpeedUp
		//
		// Many things are slow here
		// 1. The reserve of string take 25% of time
		// 2. intrinsic::divide implementation in ASM for x86 is slower than the C version
		// 3. The assignment of the character after the divide is slow, check if using a range test to determine wich character to assign can be faster
		//
		// Flamaros - 05 january 2020

		string to_string(int32_t number, int8_t base)
		{
			assert(base >= 2 && base <= 16);

			string		result;
			wchar_t*	string;
			size_t		string_length = 0;	// doesn't contains the sign
			bool		is_negative = false;

			// value range is from -2,147,483,647 to +2,147,483,647
			// without decoration we need at most 10 + 1 characters (+1 for the sign)
			reserve(result, 10 + 1);
			string = (wchar_t*)to_uft16(result);

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
						intrinsic::swap((uint16_t*)&string[i], (uint16_t*)&string[string_length - i - 1]);
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
