#include "native_generator.hpp"

#include "lexer.hpp"
#include "parser.hpp"
#include "globals.hpp"

#include <fstd/system/file.hpp>
#include <fstd/language/defer.hpp>

#include <fstd/memory/array.hpp>

#include <fstd/stream/memory_stream.hpp>

#include <fstd/system/file.hpp>

#include <fstd/os/windows/console.hpp>

#include <iostream>
#include <chrono>
#include <string>

#include <cstdlib>

using namespace std::string_literals; // enables s-suffix for std::string literals

int main(int ac, char** av)
{
    std::string             input_file_content;
    std::vector<f::Token>   tokens;
    f::AST					parsing_result;
	int						result = 0;

#if defined(OS_WINDOWS)
	fstd::os::windows::enable_default_console_configuration();
	defer { fstd::os::windows::close_console(); };
#endif

	initialize_globals();

	fstd::system::Path	path;

	fstd::system::from_native(path, LR"(.\compiler-f\main.f)"s);

    f::lex(path, tokens);
    f::parse(tokens, parsing_result);

	//std::filesystem::path output_directory_path = "./build";
	//std::filesystem::path output_file_name = "f-compiler.exe";

 //   std::filesystem::create_directory(output_directory_path);   // @Warning hacky thing, the canonical may failed if the directory doesn't exist
	//if (output_directory_path.is_relative()) {
	//	output_directory_path = std::filesystem::canonical(output_directory_path);
	//}

  //  if (c_generator::generate(output_directory_path, output_file_name, parsing_result)) {
		//return 0;
  //  }

//    generate_hello_world();

	return 1;
}
