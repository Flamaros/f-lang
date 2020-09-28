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

void test_unicode_convversions()
{
	fstd::language::string			utf8_string;
	fstd::language::UTF16LE_string	utf16_string;

	fstd::language::string			to_utf8_string;
	fstd::language::UTF16LE_string	to_utf16_string;

	defer {
		release(utf8_string);
		release(to_utf16_string);
	};

	fstd::language::assign(utf8_string, (uint8_t*)u8"a0-A9-#%-éù-€-𝄞-𠀀-£-¤");
	fstd::language::assign(utf16_string, (uint16_t*)u"a0-A9-#%-éù-€-𝄞-𠀀-£-¤");

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
	test_unicode_convversions();
}
