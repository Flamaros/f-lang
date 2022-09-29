#pragma once

#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "CPP_backend.hpp"
#include "IR_generator.hpp"
#include "instruction_db_loader.h"
#include "PE_x86_backend.hpp"

#include "lexer/lexer_base.hpp"

#include <fstd/memory/array.hpp>

#include <fstd/system/path.hpp>

namespace fstd
{
	namespace core
	{
		struct Logger;
	}
}

struct Globals
{
	fstd::core::Logger*					logger = nullptr;
	fstd::memory::Array<f::Lexer_Data>	lexer_data;
	f::Parser_Data						parser_data;
	f::CPP_Backend_Data					cpp_backend_data;
	f::IR_Data							ir_data;
	f::x86_DB::x86_DB_Data				x86_db_data;
	f::PE_X86_Backend_Data				x86_backend_data;
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
template<typename Token>
void report_error(Compiler_Error error, const Token& token, const char* error_message);
void abort_compilation();
