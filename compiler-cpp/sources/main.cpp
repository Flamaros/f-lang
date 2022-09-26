#include "PE_x86_backend.hpp"

#include "globals.hpp"

#include "IR_generator.hpp"
#include "CPP_backend.hpp"

#include "lexer/lexer.hpp"
#include "parser/parser.hpp"

#include <fstd/macros.hpp>

#include <fstd/core/string_builder.hpp>
#include <fstd/core/logger.hpp>

#include <fstd/language/defer.hpp>

#include <fstd/memory/array.hpp>

#include <fstd/system/file.hpp>
#include <fstd/system/process.hpp>

#include <fstd/os/windows/console.hpp>

#include <tracy/Tracy.hpp>
#include <tracy/common/TracySystem.hpp>

#define ENABLE_TOKENS_PRINT 0
#define ENABLE_DOT_OUTPUT 0
#define ENABLE_CPP_BACKEND 0

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
		report_error(Compiler_Error::warning, "Failed to generate the dot image. Please check that dot.exe is in your PATH.\n");
	}
}

int main(int ac, char** av)
{
	// Begin Initialization ================================================

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
	// End Initialization ================================================

	memory::Array<f::Token>	tokens;
	f::Parsing_Result		parsing_result;
	f::IR					ir;
	int						result = 0;

	if (ac != 3) {
		report_error(Compiler_Error::error, "Wrong argument number, you should specify file paths of input and output files.");
	}

	// Log compiled file
	{
		String_Builder		string_builder;
		language::string	message;

		defer {
			free_buffers(string_builder);
			release(message);
		};

		print_to_builder(string_builder, "Compiling: \"%Cs\"\n", av[1]);

		message = to_string(string_builder);
		report_error(Compiler_Error::info, (char*)to_utf8(message));
	}

	{
		ZoneScopedN("f-lang compilation");

		system::Path	path;

		defer { system::reset_path(path); };

		system::from_native(path, (const uint8_t*)av[1]);

		f::initialize_lexer();
		f::lex(path, tokens);

#if !defined(TRACY_ENABLE) && ENABLE_TOKENS_PRINT == 1
		f::print(tokens);	// Optionnal
#endif

		f::parse(tokens, parsing_result);

		// Optionnal Dot graph output
#if !defined(TRACY_ENABLE) && ENABLE_DOT_OUTPUT == 1
		{
			system::Path	ast_dot_file_path;
			system::Path	ast_png_file_path;
			system::Path	scope_dot_file_path;
			system::Path	scope_png_file_path;

			defer {
				system::reset_path(ast_dot_file_path);
				system::reset_path(ast_png_file_path);
				system::reset_path(scope_dot_file_path);
				system::reset_path(scope_png_file_path);
			};

			system::from_native(ast_dot_file_path, (uint8_t*)u8R"(.\AST.dot)");
			system::from_native(ast_png_file_path, (uint8_t*)u8R"(.\AST.png)");
			system::from_native(scope_dot_file_path, (uint8_t*)u8R"(.\scope.dot)");
			system::from_native(scope_png_file_path, (uint8_t*)u8R"(.\scope.png)");

			f::generate_dot_file(parsing_result.ast_root, ast_dot_file_path);
			convert_dot_file_to_png(ast_dot_file_path, ast_png_file_path);
			f::generate_dot_file(parsing_result.symbol_table_root, scope_dot_file_path);
			convert_dot_file_to_png(scope_dot_file_path, scope_png_file_path);
		}
#endif

		// Generate the IRL
		{
			f::generate_ir(parsing_result, ir);
		}

		// Optionnal C++ backend
#if ENABLE_CPP_BACKEND == 1
		{
			system::Path	cpp_file_path;

			defer { system::reset_path(cpp_file_path); };

			system::from_native(cpp_file_path, (uint8_t*)u8R"(.\output)");

			f::CPP_backend::compile(ir, cpp_file_path);
		}
#endif

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

		// Native backend
		{
			system::Path	output_file_path;

			defer{ system::reset_path(output_file_path); };

			system::from_native(output_file_path, (uint8_t*)av[2]);

			f::PE_x86_backend::initialize_backend(); // @TODO see to do it asynchronously (are event better at compile-time to generate C++ code with tables)
			f::PE_x86_backend::compile(ir, output_file_path);
		}
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

    f::PE_x86_backend::generate_hello_world();

	FrameMark;

	return 0;
}
