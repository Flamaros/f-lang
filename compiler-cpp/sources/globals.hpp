#pragma once

#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "IR_generator.hpp"
#include "PE_x64_backend.hpp"
#include "ASM/ASM_lexer.hpp"

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

struct Configuration
{
	bool	generate_debug_info = false;
};

struct Globals
{
	fstd::core::Logger*						logger = nullptr;
	Configuration							configuration;
	fstd::memory::Array<f::Lexer_Data>		lexer_data;
	fstd::memory::Array<f::ASM::Lexer_Data>	asm_lexer_data;
	f::Parser_Data							parser_data;
	f::IR_Data								ir_data;
	f::PE_X64_Backend_Data					x64_backend_data;
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
void report_error(Compiler_Error error, const f::ASM::Token& token, const char* error_message);
void abort_compilation();
