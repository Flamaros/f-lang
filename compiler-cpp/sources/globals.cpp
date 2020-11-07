#include "globals.hpp"

#include "lexer/lexer.hpp"

import fstd.language.string;
import fstd.language.defer;

import fstd.core.logger;
import fstd.core.string_builder;

#include <fstd/system/process.hpp>
import fstd.system.stdio;

thread_local Globals    globals;

using namespace fstd;

void initialize_globals()
{
	globals.logger = new fstd::core::Logger();
}

void report_error(Compiler_Error error, const f::Token& token, const char* error_message)
{
	core::String_Builder	string_builder;
	language::string		error_message_string;
	language::string		format_string;
	language::string		formatted_string;

	defer{
		core::free_buffers(string_builder);
		language::release(formatted_string);
	};

	language::string_view   header;
	
	if (error == Compiler_Error::error) {
		language::assign(header, (uint8_t*)"\033[38;5;196;4mError:\033[0m");
	}
	else {
		language::assign(header, (uint8_t*)"\033[38;5;208mWarning:\033[0m");
	}

	language::assign(error_message_string, (uint8_t*)error_message);
	language::assign(format_string, (uint8_t*)"%v %v(%d, %d): %s");
	core::print_to_builder(string_builder, &format_string, header, token.file_path, token.line, token.column, error_message_string);

	// @TODO print the token in a particular color

	formatted_string = core::to_string(string_builder);
	system::print(formatted_string);

	if (error == Compiler_Error::error) {
		abort_compilation();
	}
}

void abort_compilation()
{
	// @TODO stop all modules correctly to avoid file corruptions,...?
	// Ensure that all threads are stop as exit_process may deadlock otherwise.

	core::Assert(false);
	fstd::system::exit_process(1);
}
