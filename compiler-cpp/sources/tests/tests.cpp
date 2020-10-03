#include "tests.hpp"

#include <fstd/system/timer.hpp>

#include <fstd/core/assert.hpp>
#include <fstd/core/unicode.hpp>

#include <fstd/language/string.hpp>
#include <fstd/language/defer.hpp>

// @TODO remove it
#include <vector>
#include <string>
#include <cstdint>
#include <cstdlib>
#include <time.h>       /* time */

#undef max
#include <tracy/Tracy.hpp>

void test_integer_to_string_performances()
{
	ZoneScopedNC("test_integer_to_string_performances", 0xf05545);

	std::vector<int32_t>	numbers;
	uint64_t				start_time;
	uint64_t				end_time;
	double					f_implemenation_time;
	double					std_implemenation_time;

	std::srand((uint32_t)time(nullptr));

	{
		ZoneScopedNC("Initialize random numbers", 0xf05545);

		numbers.resize(10'000'000);
		for (auto& number : numbers) {
			number = std::rand();
		}
	}

	{
		ZoneScopedNC("f-lang to_string", 0xf05545);

		fstd::language::string f_string;
		start_time = fstd::system::get_time_in_nanoseconds();
		for (auto& number : numbers) {
			f_string = fstd::language::to_string(number, 10);
			release(f_string);
		}
		end_time = fstd::system::get_time_in_nanoseconds();
		f_implemenation_time = ((end_time - start_time) / 1'000'000.0);
	}

	{
		ZoneScopedNC("stl to_string", 0xf05545);

		std::string std_string;
		start_time = fstd::system::get_time_in_nanoseconds();
		for (auto& number : numbers) {
			std_string = std::to_string(number);
		}
		end_time = fstd::system::get_time_in_nanoseconds();
		std_implemenation_time = ((end_time - start_time) / 1'000'000.0);
	}

	printf("to_string performances (%llu iterations):\n    f-lang: %0.3lf ms\n    std: %0.3lf ms\n", (uint64_t)numbers.size(), f_implemenation_time, std_implemenation_time);
}

void test_unicode_code_point_convversions()
{
	size_t	peek;

	// UTF8
	uint8_t	utf8_buffer[4];

	fstd::core::Assert(fstd::core::from_utf8(0x24, 0x00, 0x00, 0x00, peek) == 0x24);		// '$'
	fstd::core::Assert(fstd::core::from_utf8(0xC3, 0xA9, 0x00, 0x00, peek) == 0xE9);		// 'é'
	fstd::core::Assert(fstd::core::from_utf8(0xE2, 0x82, 0xAC, 0x00, peek) == 0x20AC);		// '€'
	fstd::core::Assert(fstd::core::from_utf8(0xF0, 0x9D, 0x84, 0x9E, peek) == 0x1D11E);		// '𝄞'
	fstd::core::Assert(fstd::core::from_utf8(0xF0, 0x90, 0x90, 0xB7, peek) == 0x10437);		// '𐐷'
	fstd::core::Assert(fstd::core::from_utf8(0xF0, 0xA0, 0x80, 0x80, peek) == 0x20000);		// '𠀀'
	fstd::core::Assert(fstd::core::from_utf8(0xF0, 0xA4, 0xAD, 0xA2, peek) == 0x24B62);		// '𤭢'

	fstd::core::to_utf8(0x24, utf8_buffer);
	fstd::core::Assert(utf8_buffer[0] == 0x24);	// '$'
	fstd::core::to_utf8(0xE9, utf8_buffer);
	fstd::core::Assert(utf8_buffer[0] == 0xC3);	// 'é'
	fstd::core::Assert(utf8_buffer[1] == 0xA9);	// 'é'
	fstd::core::to_utf8(0x20AC, utf8_buffer);
	fstd::core::Assert(utf8_buffer[0] == 0xE2);	// '€'
	fstd::core::Assert(utf8_buffer[1] == 0x82);	// '€'
	fstd::core::Assert(utf8_buffer[2] == 0xAC);	// '€'
	fstd::core::to_utf8(0x1D11E, utf8_buffer);
	fstd::core::Assert(utf8_buffer[0] == 0xF0);	// '𝄞'
	fstd::core::Assert(utf8_buffer[1] == 0x9D);	// '𝄞'
	fstd::core::Assert(utf8_buffer[2] == 0x84);	// '𝄞'
	fstd::core::Assert(utf8_buffer[3] == 0x9E);	// '𝄞'
	fstd::core::to_utf8(0x10437, utf8_buffer);
	fstd::core::Assert(utf8_buffer[0] == 0xF0);	// '𐐷'
	fstd::core::Assert(utf8_buffer[1] == 0x90);	// '𐐷'
	fstd::core::Assert(utf8_buffer[2] == 0x90);	// '𐐷'
	fstd::core::Assert(utf8_buffer[3] == 0xB7);	// '𐐷'
	fstd::core::to_utf8(0x20000, utf8_buffer);
	fstd::core::Assert(utf8_buffer[0] == 0xF0);	// '𠀀'
	fstd::core::Assert(utf8_buffer[1] == 0xA0);	// '𠀀'
	fstd::core::Assert(utf8_buffer[2] == 0x80);	// '𠀀'
	fstd::core::Assert(utf8_buffer[3] == 0x80);	// '𠀀'
	fstd::core::to_utf8(0x24B62, utf8_buffer);
	fstd::core::Assert(utf8_buffer[0] == 0xF0);	// '𤭢'
	fstd::core::Assert(utf8_buffer[1] == 0xA4);	// '𤭢'
	fstd::core::Assert(utf8_buffer[2] == 0xAD);	// '𤭢'
	fstd::core::Assert(utf8_buffer[3] == 0xA2);	// '𤭢'

	// UTF16
	uint16_t	utf16_buffer[2];

	fstd::core::Assert(fstd::core::from_utf16LE(0x0024, 0x00, peek) == 0x24);		// '$'
	fstd::core::Assert(fstd::core::from_utf16LE(0x00E9, 0x00, peek) == 0xE9);		// 'é'
	fstd::core::Assert(fstd::core::from_utf16LE(0x20AC, 0x00, peek) == 0x20AC);		// '€'
	fstd::core::Assert(fstd::core::from_utf16LE(0xD834, 0xDD1E, peek) == 0x1D11E);	// '𝄞'
	fstd::core::Assert(fstd::core::from_utf16LE(0xD801, 0xDC37, peek) == 0x10437);	// '𐐷'
	fstd::core::Assert(fstd::core::from_utf16LE(0xD840, 0xDC00, peek) == 0x20000);	// '𠀀'
	fstd::core::Assert(fstd::core::from_utf16LE(0xD852, 0xDF62, peek) == 0x24B62);	// '𤭢'

	fstd::core::to_utf16LE(0x24, utf16_buffer);
	fstd::core::Assert(utf16_buffer[0] == 0x0024);	// '$'
	fstd::core::to_utf16LE(0xE9, utf16_buffer);
	fstd::core::Assert(utf16_buffer[0] == 0x00E9);	// 'é'
	fstd::core::to_utf16LE(0x20AC, utf16_buffer);
	fstd::core::Assert(utf16_buffer[0] == 0x20AC);	// '€'
	fstd::core::to_utf16LE(0x1D11E, utf16_buffer);
	fstd::core::Assert(utf16_buffer[0] == 0xD834);	// '𝄞'
	fstd::core::Assert(utf16_buffer[1] == 0xDD1E);	// '𝄞'
	fstd::core::to_utf16LE(0x10437, utf16_buffer);
	fstd::core::Assert(utf16_buffer[0] == 0xD801);	// '𐐷'
	fstd::core::Assert(utf16_buffer[1] == 0xDC37);	// '𐐷'
	fstd::core::to_utf16LE(0x20000, utf16_buffer);
	fstd::core::Assert(utf16_buffer[0] == 0xD840);	// '𠀀'
	fstd::core::Assert(utf16_buffer[1] == 0xDC00);	// '𠀀'
	fstd::core::to_utf16LE(0x24B62, utf16_buffer);
	fstd::core::Assert(utf16_buffer[0] == 0xD852);	// '𤭢'
	fstd::core::Assert(utf16_buffer[1] == 0xDF62);	// '𤭢'
}

void test_unicode_string_convversions()
{
	fstd::language::string			utf8_string;
	fstd::language::UTF16LE_string	utf16_string;

	fstd::language::string			to_utf8_string;
	fstd::language::UTF16LE_string	to_utf16_string;

	defer {
		release(utf8_string);
		release(utf16_string);
		release(to_utf8_string);
		release(to_utf16_string);
	};

	fstd::language::assign(utf8_string, (uint8_t*)u8"a0-A9-#%-éù-€-𝄞-𠀀-£-¤");
	fstd::language::assign(utf16_string, (uint16_t*)u"a0-A9-#%-éù-€-𝄞-𠀀-£-¤");
	// Hexa: 61 30  2D  41 39  2D  23 25  2D  E9  F9   2D  20AC	 2D  1D11E   2D  20000   2D  A3  A4
	// Deci: 97 48  45  65 57  45  35 37  45  233 249  45  8364	 45  119070  45  131072  45  163 164

	fstd::core::from_utf8_to_utf16LE(utf8_string, to_utf16_string, true);
	fstd::core::from_utf16LE_to_utf8(utf16_string, to_utf8_string, true);

	fstd::core::Assert(are_equals(to_utf16_string, utf16_string));
	fstd::core::Assert(are_equals(to_utf8_string, utf8_string));
}

void run_tests()
{
	ZoneScopedNC("run_tests", 0xb71c1c);

#if !defined(_DEBUG)
	test_integer_to_string_performances();
#endif
	test_unicode_code_point_convversions();
	test_unicode_string_convversions();
}
