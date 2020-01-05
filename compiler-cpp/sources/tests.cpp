#include "tests.hpp"

#include <fstd/system/timer.hpp>

#include <fstd/language/string.hpp>

#include <vector>
#include <string>

#include <cstdint>
#include <cstdlib>

#include <time.h>       /* time */

void test_integer_to_string_performances()
{
	std::vector<int32_t>	numbers;
	uint64_t				start_time;
	uint64_t				end_time;
	double					f_implemenation_time;
	double					std_implemenation_time;

	std::srand((uint32_t)time(nullptr));

	numbers.resize(10'000'000);
	for (auto& number : numbers) {
		number = std::rand();
	}

	fstd::language::string f_string;
	start_time = fstd::system::get_time_in_nanoseconds();
	for (auto& number : numbers) {
		f_string = fstd::language::to_string(number, 10);
		release(f_string);
	}
	end_time = fstd::system::get_time_in_nanoseconds();
	f_implemenation_time = ((end_time - start_time) / 1000000.0);

	std::string std_string;
	start_time = fstd::system::get_time_in_nanoseconds();
	for (auto& number : numbers) {
		std_string = std::to_string(number);
	}
	end_time = fstd::system::get_time_in_nanoseconds();
	std_implemenation_time = ((end_time - start_time) / 1000000.0);

	printf("\nto_string performances (%llu iterations):\n    f-lang: %0.3lf ms\n    std: %0.3lf ms\n", (uint64_t)numbers.size(), f_implemenation_time, std_implemenation_time);
}

void run_tests()
{
	test_integer_to_string_performances();
}