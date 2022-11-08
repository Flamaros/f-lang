#pragma once

#include "lexer/lexer_base.hpp"

#include <fstd/memory/array.hpp>

#include <fstd/language/types.hpp>

namespace f
{
    namespace x86_DB
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

        struct x86_DB_Data
        {
            fstd::memory::Array<Token<x86_Keyword>>	tokens;
            fstd::memory::Array<uint8_t>			instructions;
        };

        // Struct for database entries (used for machine generation)
        struct Instruction
        {
            struct Operand
            {
                enum class Type : uint8_t // @TODO UPPER_CASE
                {
                    Unused = 0x00,
                    Register = 0x01,
                    MemoryAddress = 0x02,
                    ImmediateValue = 0x04,
                };
                Type    type;
                uint8_t size; // size of Register or ImmediateValue, 0 for MemoryAddress and Unused types
            };

            struct Translation_Instructions
            {
                // [mi:    hle o32 83 /0 ib,s] 

                // continue to add flags, also check the nb bits per value,...
                // https://softwareengineering.stackexchange.com/questions/227983/how-do-we-go-from-assembly-to-machine-codecode-generation/320297#320297?newreg=a4771182c1c240d1afbbc58d28b90574
                //

                bool lock_prefix;
                // bool output32_prefix; // we don't care about o32 because it's for 16bit output format, we'll only support 32 and 64 bit formats 

                uint32_t opcode : 8;

                // modr/m byte
                //uint8_t mod        : 2; // 00 => indirect, e.g. [eax]
                //                        // 01 = > indirect plus byte offset
                //                        // 10 = > indirect plus word offset
                //                        // 11 = > register
                //uint8_t reg        : 3; // identifies register
                //uint8_t rm         : 3; // identifies second register or additional data

                uint8_t extra_data; // decimal value to put in rm when it is not used. The value is in a range of 0-7.

                union
                {
                    uint8_t _unsigned;
                    int8_t  _signed;
                }   immediate_byte;
            };

            fstd::language::string_view name;
            Operand                     operands[3];    // x86 instructions can have 0 to 3 operands
            Translation_Instructions    translation_instructions;
        };

        void load_x86_instruction_DB();
    }
}
