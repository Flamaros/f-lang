#include "native_generator.hpp"

#include "globals.hpp"
#include "macros.hpp"

#include "IR_generator.hpp"
#include "CPP_backend.hpp"

#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "tests/tests.hpp"

#include <fstd/core/string_builder.hpp>
#include <fstd/core/logger.hpp>

#include <fstd/language/defer.hpp>

#include <fstd/memory/array.hpp>

#include <fstd/system/file.hpp>
#include <fstd/system/process.hpp>

#include <fstd/os/windows/console.hpp>

#undef max
#include <tracy/Tracy.hpp>
#include <tracy/common/TracySystem.hpp>

using namespace fstd;
using namespace fstd::core;

void convert_dot_file_to_png(const system::Path& dot_file_path, const system::Path& png_file_path)
{
	ZoneScopedN("convert_dot_file_to_png");

	String_Builder	string_builder;
	system::Path	dot_executable_file_path;

	defer{
		free_buffers(string_builder);
	};

	defer{
		system::reset_path(dot_executable_file_path);
	};

	system::from_native(dot_executable_file_path, (uint8_t*)u8R"(dot.exe)");

	print_to_builder(string_builder, "%s -Tpng -o %s", dot_file_path, png_file_path);
	if (!system::execute_process(dot_executable_file_path, to_string(string_builder))) {
		f::Token dummy_token;

		dummy_token.type = f::Token_Type::UNKNOWN;
		dummy_token.file_path = system::to_string(dot_executable_file_path);
		dummy_token.line = 0;
		dummy_token.column = 0;
		report_error(Compiler_Error::warning, dummy_token, "Failed to generate the dot image. Please check that dot.exe is in your PATH.\n");
	}
}

int main(int ac, char** av)
{
	run_tests();

	memory::Array<f::Token>	tokens;
    f::AST					parsing_result;
	f::IR					ir;
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

		defer { system::reset_path(path); };

		system::from_native(path, (uint8_t*)u8R"(.\compiler-f\main.f)");

		f::initialize_lexer();
		f::lex(path, tokens);
		f::print(tokens);	// Optionnal

		f::parse(tokens, parsing_result);

		// Optionnal Dot graph output
		{
			system::Path	dot_file_path;
			system::Path	png_file_path;

			defer {
				system::reset_path(dot_file_path);
				system::reset_path(png_file_path);
			};

			system::from_native(dot_file_path, (uint8_t*)u8R"(.\AST.dot)");
			system::from_native(png_file_path, (uint8_t*)u8R"(.\AST.png)");

			f::generate_dot_file(parsing_result, dot_file_path);
			convert_dot_file_to_png(dot_file_path, png_file_path);
		}

		// Generate the IRL
		{
			f::generate_ir(parsing_result, ir);
		}

		// Optionnal C++ backend
		{
			system::Path	cpp_file_path;

			defer { system::reset_path(cpp_file_path); };

			system::from_native(cpp_file_path, (uint8_t*)u8R"(.\output)");

			f::CPP_backend::compile(ir, cpp_file_path);
		}

		// @TODO Evaluate all constant expressions.
		//
		// We need to evaluate all constant expressions before the type deduction pass. This is necessary to have a proper
		// type deduction with a good error checking (sign mismatch, type size,...).
		// All operations on numeric literals should be processed as cast operations.
		// This also impact the array that have a fixed size. this is important for function parameters,...
		// There is also values of enums to compute.
		//
		// Check if it should be done for string literals too.
		// My_Enum::names[enum_value] is also a string literal.
		// With string literals is seems a little harder without operators like '+', as for consitancy between compile-time and runtime
		// code evaluation it certainly should works with String_Builder, but it doesn't seems really pratical and easy to implement. Maybe
		// having the '+' operator that works only at compile time with string literals is OK.
		// Here with string literals it seems to be only an optimization pass more than something really useful for type deduction and error
		// reporting.
		//
		// Flamaros - 07 may 2020

		// @TODO Type deduction pass
		//
		// Compute size of structs,...
		// Check that enum values are in the expected range,...
		// Infer variable types.
		//
		// Flamaros - 07 may 2020
	}


	// @TODO Code generation pass
	//
	// Flamaros - 07 may 2020


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
