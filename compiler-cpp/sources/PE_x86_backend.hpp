#pragma once

#include "IR_generator.hpp"

#include <fstd/system/path.hpp>
#include <fstd/memory/array.hpp>
#include <fstd/memory/hash_table.hpp>

namespace f
{
    enum class x86_Keyword : uint8_t
    {
        UNKNOWN,

        _IGNORE,

        // @TODO document
        RESB,

        // Operand types
        _VOID,
        REG8,
        REG16,
        REG32,
        REG64,
        REG_AL,
        reg_ax,
        reg_eax,
        reg_rax,
        reg_dx,
        MEM,
        MEM16,
        MEM32,
        MEM64,
        mem80,
        IMM,
        IMM8,
        IMM16,
        IMM32,
        IMM64,
        SBYTEWORD,
        SBYTEWORD16,
        SBYTEWORD32,
        fpureg,
        fpu0,

        // Operand modifier
        _NEAR,
        _FAR,
        TO
    };

    struct PE_X86_Backend_Data
	{
		fstd::memory::Array<Token<x86_Keyword>>	tokens;
		fstd::memory::Array<uint8_t>			instructions;
	};

	namespace PE_x86_backend
	{
		bool generate_hello_world(); // @TODO remove it

		void initialize_backend();
		void compile(IR& ir, const fstd::system::Path& output_file_path);
	}
}
