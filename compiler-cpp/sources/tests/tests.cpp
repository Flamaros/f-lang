#include "tests.hpp"

#include <fstd/system/timer.hpp>

#include <fstd/language/string.hpp>

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

void run_tests()
{
	ZoneScopedNC("run_tests", 0xb71c1c);

	test_integer_to_string_performances();
}
