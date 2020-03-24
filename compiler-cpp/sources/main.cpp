#include "native_generator.hpp"

#include "globals.hpp"
#include "macros.hpp"

#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "tests/tests.hpp"

#include <fstd/core/string_builder.hpp>
#include <fstd/core/logger.hpp>

#include <fstd/language/defer.hpp>

#include <fstd/memory/array.hpp>

#include <fstd/system/file.hpp>

#include <fstd/os/windows/console.hpp>

#undef max
#include <tracy/Tracy.hpp>
#include <tracy/common/TracySystem.hpp>

using namespace fstd;

int main(int ac, char** av)
{
	memory::Array<f::Token>	tokens;
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

#if defined(FSTD_DEBUG)
	core::set_log_level(*globals.logger, core::Log_Level::verbose);
#endif

	FrameMark;
	// Initialization and tests ================================================

	{
		ZoneScopedN("f-lang parsing");

		system::Path	path;

		defer{ system::reset_path(path); };

		system::from_native(path, (uint8_t*)u8R"(.\compiler-f\main.f)");

		f::initialize_lexer();
		f::lex(path, tokens);
		f::print(tokens);	// Optionnal


		system::Path	dot_file_path;

		defer{ system::reset_path(dot_file_path); };

		system::from_native(dot_file_path, (uint8_t*)u8R"(.\AST.dot)");

		f::parse(tokens, parsing_result);
		f::generate_dot_file(parsing_result, dot_file_path);	// Optionnal
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

	return 0;
}
