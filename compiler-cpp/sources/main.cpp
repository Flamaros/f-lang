#include "native_generator.hpp"

#include "lexer.hpp"
#include "parser.hpp"
#include "c_generator.hpp"
#include "globals.hpp"

#include <utilities/file.hpp>
#include <utilities/string.hpp>
#include <utilities/exit_scope.hpp>

#include <fstd/memory/array.hpp>

#include <fstd/stream/memory_stream.hpp>

#include <fstd/system/file.hpp>

#include <fstd/os/windows/console.hpp>

#include <iostream>
#include <chrono>
#include <string>

#include <cstdlib>

using namespace std::string_literals; // enables s-suffix for std::string literals

#define TEST_PARSING_SPEED

void	test_parsing_speed()
{
	typedef std::chrono::high_resolution_clock	Clock;
	typedef std::chrono::time_point<Clock>		Time_Point;

	Time_Point	start;
	Time_Point	start_lexing;
	Time_Point	start_parsing;
	Time_Point	end;
	Time_Point	end_lexing;
	Time_Point	end_parsing;

	start = Clock::now();

	std::vector<f::Token>   tokens;
	f::AST					parsing_result;
	int						result = 0;

	fstd::system::Path	path;

	fstd::system::from_native(path, LR"(.\tests\big_file.f)"s);

	start_lexing = Clock::now();
	f::lex(path, tokens);
	end_lexing = Clock::now();

	start_parsing = Clock::now();
	f::parse(tokens, parsing_result);
	end_parsing = Clock::now();

	end = Clock::now();

	size_t	nb_lines = tokens.back().line;

	std::chrono::duration<double> lexing_s = end_lexing - start_lexing;
	std::chrono::duration<double> parsing_s = end_parsing - start_parsing;
	std::chrono::duration<double> total_s = end - start;

	std::cout << "./tests/big_file.f" << std::endl;
	std::cout << "    lexing:   " << lexing_s.count()  << " s - lines/s: " << nb_lines / lexing_s.count() << std::endl;
	std::cout << "    parsing:  " << parsing_s.count() << " s - lines/s: " << nb_lines / parsing_s.count() << std::endl;
	std::cout << "    total:    " << total_s.count()   << " s - lines/s: " << nb_lines / total_s.count() << std::endl;
}

int main(int ac, char** av)
{
    std::string             input_file_content;
    std::vector<f::Token>   tokens;
    f::AST					parsing_result;
	int						result = 0;

#if defined(PLATFORM_WINDOWS)
	fstd::os::windows::enable_default_console_configuration();
	defer { fstd::os::windows::close_console(); };
#endif

	initialize_globals();

#if defined(TEST_PARSING_SPEED)
	test_parsing_speed();
#endif

    if (utilities::read_all_file("./compiler-f/main.f", input_file_content) == false) {
        return 1;
    }

	fstd::system::Path	path;

	fstd::system::from_native(path, LR"(.\compiler-f\main.f)"s);

    f::lex(path, tokens);
    f::parse(tokens, parsing_result);

	std::filesystem::path output_directory_path = "./build";
	std::filesystem::path output_file_name = "f-compiler.exe";

    std::filesystem::create_directory(output_directory_path);   // @Warning hacky thing, the canonical may failed if the directory doesn't exist
	if (output_directory_path.is_relative()) {
		output_directory_path = std::filesystem::canonical(output_directory_path);
	}

    if (c_generator::generate(output_directory_path, output_file_name, parsing_result)) {
		return 0;
    }

//    generate_hello_world();

	return 1;
}
