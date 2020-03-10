#pragma once

#include "lexer/lexer.hpp"

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
};

void initialize_globals();

extern thread_local Globals    globals;

// Functions that change the global execution
enum class Compiler_Error
{
	warning,
	error
};

void report_error(Compiler_Error error, const f::Token& token, const char* error_message);
void abort_compilation();
