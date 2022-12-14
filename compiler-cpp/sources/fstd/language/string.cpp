#include "string.hpp"

#include <fstd/language/intrinsic.hpp>

#include <fstd/core/assert.hpp>

#include <type_traits>

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

		static const uint8_t* padding_buffer = (uint8_t*)
			"0000000000000000000000000000000000000000000000000000000000000000"; // 64 zeros

		// Inspiration
		// https://github.com/fmtlib/fmt/blob/master/include/fmt/format.h
		//
		// @SpeedUp:
		// 1. intrinsic::divide implementation in ASM for x86 is slower than the C version
		//
		// Flamaros - 23 january 2020

		template void to_string<int32_t>(int32_t number, string& output);
		template void to_string<uint32_t>(uint32_t number, string& output);
		template void to_string<int64_t>(int64_t number, string& output);
		template void to_string<uint64_t>(uint64_t number, string& output);

		template<typename IntegerType>
		void to_string(IntegerType number, string& output)
		{
			static_assert(std::is_same<IntegerType, int32_t>::value ||
						std::is_same<IntegerType, uint32_t>::value ||
						std::is_same<IntegerType, int64_t>::value ||
						std::is_same<IntegerType, uint64_t>::value);
			static_assert(std::is_integral<IntegerType>::value, "to_string works only with integral types.");

			uint8_t*	string;
			size_t		string_length = 0;	// doesn't contains the sign
			bool		is_negative = false;

			// value range is from -9,223,372,036,854,775,807 to +9,223,372,036,854,775,807
			// without decoration we need at most 19 + 1 characters (+1 for the sign)
			reserve(output, 19 + 1);
			string = (uint8_t*)to_utf8(output);

			// @TODO May we optimize this code?
			{
				IntegerType	quotien = number;
				IntegerType	reminder;

				if constexpr (std::is_same<IntegerType, int32_t>::value ||
							std::is_same<IntegerType, int64_t>::value)
				{
					if (number < 0) {
						is_negative = true;
						string[0] = '-';
						string++;
						quotien = -number;
					}
				}

				while (quotien >= 100) {
					// Integer division is slow so do it for a group of two digits instead
					// of for every digit. The idea comes from the talk by Alexandrescu
					// "Three Optimization Tips for C++". See speed-test for a comparison.

//					intrinsic::divide(quotien, 100, &quotien, &reminder);
					reminder = quotien % (uint64_t)100;
					quotien = quotien / (uint64_t)100;

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

			resize(output, string_length + is_negative);
		}

		template void to_string<uint32_t>(uint32_t number, int8_t base, string& output);
		template void to_string<uint64_t>(uint64_t number, int8_t base, string& output);

		// Should I put the base as template parameter for better performances?
		template<typename IntegerType>
		void to_string(IntegerType number, int8_t base, string& output)
		{
			static_assert(std::is_same<IntegerType, uint32_t>::value ||
						std::is_same<IntegerType, uint64_t>::value);
			static_assert(std::is_integral<IntegerType>::value, "to_string works only with integral types.");

			fstd::core::Assert(base >= 2 && base <= 16);
			
			// For base 10 we fallback on the specific implementation which handle the sign and is fastest
			if (base == 10) {
				to_string(number, output);
				return;
			}

			uint8_t*	string;
			int32_t		string_length = 0;	// signed integer to correctly handle the padding test

			// We can have up to 64 characters when printing in binary a 64 bits number
			reserve(output, 64);
			string = (uint8_t*)to_utf8(output);

			// @TODO May we optimize this code?
			{
				IntegerType	quotien = number;
				IntegerType	reminder;

				if (number) {	// we can't divide 0
					do
					{
						reminder = quotien % base;
						quotien = quotien / base;

						string[string_length++] = ordered_digits[reminder];
					} while (quotien);

					// Padding with 0
					int32_t padding_size = 0;
					if constexpr (std::is_same<IntegerType, uint32_t>::value) // 32 bits
					{
						if (base == 2) {
							padding_size = 32 - string_length;
						}
						else if (base == 16) {
							padding_size = 8 - string_length;
						}
					}
					else // 64 bits
					{
						if (base == 2) {
							padding_size = 64 - string_length;
						}
						else if (base == 16) {
							padding_size = 16 - string_length;
						}
					}

					if (padding_size > 0) {
						system::memory_copy(&string[string_length], padding_buffer, padding_size);
						string_length += padding_size;
					}

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

			resize(output, string_length);
		}
	}
}
