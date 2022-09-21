#pragma once

#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "CPP_backend.hpp"
#include "IR_generator.hpp"

#include <fstd/memory/array.hpp>

#include <fstd/system/path.hpp>

namespace fstd
{
	namespace core
	{
		struct Logger;
	}
}

namespace f
{
	struct Token;
}

struct Globals
{
	fstd::core::Logger*					logger = nullptr;
	fstd::memory::Array<f::Lexer_Data>	lexer_data;
	f::Parser_Data						parser_data;
	f::CPP_Backend_Data					cpp_backend_data;
	f::IR_Data							ir_data;
};

void initialize_globals();

extern thread_local Globals    globals;

// Functions that change the global execution
enum class Compiler_Error
{
	info,
	warning,
	error,
	internal_error // Use it when the use trigger a limitation of the implementation of the compiler
};

void report_error(Compiler_Error error, const char* error_message);
void report_error(Compiler_Error error, const f::Token& token, const char* error_message);
void abort_compilation();
