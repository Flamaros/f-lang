#pragma once

#include "ASM/ASM.hpp"

#include <fstd/system/path.hpp>

namespace f
{
    enum class x64_Keyword : uint8_t
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

    struct PE_X64_Backend_Data
	{
	};

	namespace PE_x64_backend
	{
		bool generate_hello_world(); // @TODO remove it

		void initialize_backend();
		void compile(const ASM::ASM& asm_result, const fstd::system::Path& output_file_path);
	}
}
