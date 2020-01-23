#include "native_generator.hpp"

#include "lexer.hpp"
#include "parser.hpp"
#include "globals.hpp"
#include "tests.hpp"

#include <fstd/core/string_builder.hpp>

#include <fstd/language/defer.hpp>

#include <fstd/memory/array.hpp>

#include <fstd/stream/memory_stream.hpp>

#include <fstd/system/file.hpp>

#include <fstd/os/windows/console.hpp>

#undef max
#include <tracy/Tracy.hpp>
#include <tracy/common/TracySystem.hpp>

using namespace std::string_literals; // enables s-suffix for std::string literals

using namespace fstd;

int main(int ac, char** av)
{
    std::string             input_file_content;
    std::vector<f::Token>   tokens;
    f::AST					parsing_result;
	int						result = 0;

	system::allocator_initialize();

#if defined(FSTD_OS_WINDOWS)
	os::windows::enable_default_console_configuration();
	defer { os::windows::close_console(); };
#endif

#if defined(TRACY_ENABLE)
	tracy::SetThreadName("Main thread");
#endif

	initialize_globals();

	core::String_Builder	string_builder;
	language::string		format;
	language::string		formatted_string;

	defer{
		core::free_buffers(string_builder);
		language::release(formatted_string);
	};

	language::assign(format, L"test: %d %d %d\n");
	core::print_to_builder(string_builder, &format, -12340, 1234, 12340);

	formatted_string = core::to_string(string_builder);
	system::print(formatted_string);

	run_tests();

	FrameMark;
	// Initialization and tests ================================================

	{
		ZoneScopedN("f-lang parsing");

		system::Path	path;

		defer{ system::reset_path(path); };

		system::from_native(path, LR"(.\compiler-f\main.f)"s);

		f::lex(path, tokens);
		f::parse(tokens, parsing_result);
	}

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


	FrameMark;

	return 1;
}
