#include "native_generator.hpp"

#include "lexer.hpp"
#include "parser.hpp"
#include "c_generator.hpp"

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
	Time_Point	start_tokenize;
	Time_Point	start_parse;
	Time_Point	end;
	Time_Point	end_tokenize;
	Time_Point	end_parse;

	start = Clock::now();

	std::string             input_file_content;
	std::vector<f::Token>   tokens;
	f::AST					parsing_result;
	int						result = 0;

	if (utilities::read_all_file("./tests/big_file.f", input_file_content) == true) {
		start_tokenize = Clock::now();
		f::tokenize(input_file_content, tokens);
		end_tokenize = Clock::now();

		start_parse = Clock::now();
		f::parse(tokens, parsing_result);
		end_parse = Clock::now();
	}

	end = Clock::now();

	size_t	nb_lines = tokens.back().line;

	std::chrono::duration<double> tokenize_s = end_tokenize - start_tokenize;
	std::chrono::duration<double> parse_s = end_parse - start_parse;
	std::chrono::duration<double> total_s = end - start;

	std::cout << "./tests/big_file.f" << std::endl;
	std::cout << "    tokenize: " << tokenize_s.count() << " s - lines/s: " << nb_lines / tokenize_s.count() << std::endl;
	std::cout << "    parse:    " << parse_s.count() << " s - lines/s: " << nb_lines / parse_s.count() << std::endl;
	std::cout << "    total:    " << total_s.count() << " s - lines/s: " << nb_lines / total_s.count() << std::endl;
}

int main(int ac, char** av)
{
    std::string             input_file_content;
    std::vector<f::Token>   tokens;
    f::AST					parsing_result;
	int						result = 0;

#if defined(PLATFORM_WINDOWS)
	fstd::os::windows::enable_default_console_configuration();
#endif

	defer{ 

#if defined(PLATFORM_WINDOWS)
	fstd::os::windows::close_console();
#endif
	};

#if defined(TEST_PARSING_SPEED)
	test_parsing_speed();
#endif

    if (utilities::read_all_file("./compiler-f/main.f", input_file_content) == false) {
        return 1;
    }

	fstd::system::File	file;
	fstd::system::Path	path;

	fstd::system::from_native(path, LR"(.\compiler-f\main.f)"s);
	fstd::system::from_native(path, LR"(C:\Users\Xavier\Documents\development\f-lang\compiler-f\main.f)"s);

	fstd::system::open_file(file, path, fstd::system::File::Opening_Flag::READ);
	fstd::memory::Array<uint8_t>	source_file_content = fstd::system::get_file_content(file);

	fstd::stream::Memory_Stream	stream;

	fstd::stream::initialize_memory_stream(stream, source_file_content);
	bool has_utf8_boom = fstd::stream::is_uft8_bom(stream, true);

    f::tokenize(input_file_content, tokens);
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
