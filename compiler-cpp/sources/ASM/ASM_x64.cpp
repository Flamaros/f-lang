#include "ASM_x64.h"

#include "ASM.hpp"

// @TODO should be generated by a parser of insns.dat and regs.dat

namespace f::ASM
{
	size_t g_instruction_desc_table_indices[(size_t)Instruction::COUNT + 1] = {
		0,	// UNKNOWN
		0,	// ADD - UNKNOWN has 0 desc
		2,	// CALL
		3,	// HLT
		4,	// MOV
		8,	// PUSH
		10,	// SUB
		12
	};

	Instruction_Desc g_instruction_desc_table[] = {
		// UNKNOWN

		// ADD
		{0x83, {Operand::Type::REGISTER, Operand::Size::QUAD_WORD}, {Operand::Type::IMMEDIATE, Operand::Size::BYTE}}, // ADD     rm64,imm8           [mi:    hle o64 83 /0 ib,s]         X64,LOCK
		{0x16, {Operand::Type::ADDRESS, Operand::Size::WORD}},

		// CALL
		{0xFF, {Operand::Type::ADDRESS, Operand::Size::QUAD_WORD}, {Operand::Type::NONE, Operand::Size::NONE}},	// CALL        mem64|far           [m: o64 ff /3]              X64

		// HLT
		{0xF4},	// HLT     void                [   f4]                 8086,PRIV

		// MOV
		{0xB8, {Operand::Type::REGISTER, Operand::Size::DOUBLE_WORD}, {Operand::Type::IMMEDIATE, Operand::Size::BYTE}}, // ??? seems to be a double immediate at least: MOV     reg32,imm           [ri:    o32 b8+r id]                386,SM
		{0xB8, {Operand::Type::REGISTER, Operand::Size::QUAD_WORD}, {Operand::Type::IMMEDIATE, Operand::Size::BYTE}}, // ??? seems to be a double immediate at least: MOV     reg32,imm           [ri:    o32 b8+r id]                386,SM
		{0x8B, {Operand::Type::REGISTER, Operand::Size::QUAD_WORD}, {Operand::Type::REGISTER, Operand::Size::QUAD_WORD}}, // MOV     reg64,reg64[rm:o64 8b / r]              X64
		{0x8B, {Operand::Type::REGISTER, Operand::Size::QUAD_WORD}, {Operand::Type::ADDRESS, Operand::Size::QUAD_WORD}}, // MOV     reg64,mem           [rm:    o64 8b /r]              X64,SM

		// PUSH
		{0x6A, {Operand::Type::IMMEDIATE, Operand::Size::BYTE}}, // PUSH        imm8                [i: 6a ib,s]                    186
		{0x16, {Operand::Type::ADDRESS, Operand::Size::WORD}},

		// SUB
		{0x83, {Operand::Type::REGISTER, Operand::Size::QUAD_WORD}, {Operand::Type::IMMEDIATE, Operand::Size::BYTE}}, // SUB     rm64,imm8           [mi:    hle o64 83 /5 ib,s]         X64,LOCK
		{0x16, {Operand::Type::ADDRESS, Operand::Size::WORD}},
	};

	Register_Desc	g_register_desc_table[(size_t)Register::COUNT] = {
		{Operand::Size::NONE, 0},	// UNKNOWN,

		// General - purpose registe		rs
		{Operand::Size::BYTE, 0},			// AL,
		{Operand::Size::BYTE, 0},			// AH,
		{Operand::Size::WORD, 0},			// AX,
		{Operand::Size::DOUBLE_WORD, 0},	// EAX,
		{Operand::Size::QUAD_WORD, 0},		// RAX,
		{Operand::Size::BYTE, 0},			// BL,
		{Operand::Size::BYTE, 0},			// BH,
		{Operand::Size::WORD, 0},			// BX,
		{Operand::Size::DOUBLE_WORD, 0},	// EBX,
		{Operand::Size::QUAD_WORD, 0},		// RBX,
		{Operand::Size::BYTE, 0},			// CL,
		{Operand::Size::BYTE, 0},			// CH,
		{Operand::Size::WORD, 0},			// CX,
		{Operand::Size::DOUBLE_WORD, 0},	// ECX,
		{Operand::Size::QUAD_WORD, 0},		// RCX,
		{Operand::Size::BYTE, 0},			// DL,
		{Operand::Size::BYTE, 0},			// DH,
		{Operand::Size::WORD, 0},			// DX,
		{Operand::Size::DOUBLE_WORD, 0},	// EDX,
		{Operand::Size::QUAD_WORD, 0},		// RDX,
		{Operand::Size::BYTE, 0},			// SPL,
		{Operand::Size::WORD, 0},			// SP,
		{Operand::Size::DOUBLE_WORD, 0},	// ESP,
		{Operand::Size::QUAD_WORD, 0},		// RSP,
		{Operand::Size::BYTE, 0},			// BPL,
		{Operand::Size::WORD, 0},			// BP,
		{Operand::Size::DOUBLE_WORD, 0},	// EBP,
		{Operand::Size::QUAD_WORD, 0},		// RBP,
		{Operand::Size::BYTE, 0},			// SIL,
		{Operand::Size::WORD, 0},			// SI,
		{Operand::Size::DOUBLE_WORD, 0},	// ESI,
		{Operand::Size::QUAD_WORD, 0},		// RSI,
		{Operand::Size::BYTE, 0},			// DIL,
		{Operand::Size::WORD, 0},			// DI,
		{Operand::Size::DOUBLE_WORD, 0},	// EDI,
		{Operand::Size::QUAD_WORD, 0},		// RDI,
		{Operand::Size::QUAD_WORD, 0},		// R8
		{Operand::Size::DOUBLE_WORD, 0},	// R8D,
		{Operand::Size::WORD, 0},			// R8W,
		{Operand::Size::BYTE, 0},			// R8B,
		{Operand::Size::QUAD_WORD, 0},		// R9,
		{Operand::Size::DOUBLE_WORD, 0},	// R9D,
		{Operand::Size::WORD, 0},			// R9W,
		{Operand::Size::BYTE, 0},			// R9B,
		{Operand::Size::QUAD_WORD, 0},		// R10,
		{Operand::Size::DOUBLE_WORD, 0},	// R10D,
		{Operand::Size::WORD, 0},			// R10W,
		{Operand::Size::BYTE, 0},			// R10B,
		{Operand::Size::QUAD_WORD, 0},		// R11,
		{Operand::Size::DOUBLE_WORD, 0},	// R11D,
		{Operand::Size::WORD, 0},			// R11W,
		{Operand::Size::BYTE, 0},			// R11B,
		{Operand::Size::QUAD_WORD, 0},		// R12,
		{Operand::Size::DOUBLE_WORD, 0},	// R12D,
		{Operand::Size::WORD, 0},			// R12W,
		{Operand::Size::BYTE, 0},			// R12B,
		{Operand::Size::QUAD_WORD, 0},		// R13,
		{Operand::Size::DOUBLE_WORD, 0},	// R13D,
		{Operand::Size::WORD, 0},			// R13W,
		{Operand::Size::BYTE, 0},			// R13B,
		{Operand::Size::QUAD_WORD, 0},		// R14,
		{Operand::Size::DOUBLE_WORD, 0},	// R14D,
		{Operand::Size::WORD, 0},			// R14W,
		{Operand::Size::BYTE, 0},			// R14B,
		{Operand::Size::QUAD_WORD, 0},		// R15,
		{Operand::Size::DOUBLE_WORD, 0},	// R15D,
		{Operand::Size::WORD, 0},			// R15W,
		{Operand::Size::BYTE, 0},			// R15B,

		// Segment registers
		{Operand::Size::QUAD_WORD, 0},	// ES,
		{Operand::Size::QUAD_WORD, 0},	// CS,
		{Operand::Size::QUAD_WORD, 0},	// SS,
		{Operand::Size::QUAD_WORD, 0},	// DS,
		{Operand::Size::QUAD_WORD, 0},	// FS,
		{Operand::Size::QUAD_WORD, 0},	// GS,
		{Operand::Size::QUAD_WORD, 0},	// SEGR6,
		{Operand::Size::QUAD_WORD, 0},	// SEGR7,

		// Control registers
		{Operand::Size::QUAD_WORD, 0},	// CR0,
		{Operand::Size::QUAD_WORD, 0},	// CR1,
		{Operand::Size::QUAD_WORD, 0},	// CR2,
		{Operand::Size::QUAD_WORD, 0},	// CR3,
		{Operand::Size::QUAD_WORD, 0},	// CR4,
		{Operand::Size::QUAD_WORD, 0},	// CR5,
		{Operand::Size::QUAD_WORD, 0},	// CR6,
		{Operand::Size::QUAD_WORD, 0},	// CR7,
		{Operand::Size::QUAD_WORD, 0},	// CROperand::Size::Byte,
		{Operand::Size::QUAD_WORD, 0},	// CR9,
		{Operand::Size::QUAD_WORD, 0},	// CR10,
		{Operand::Size::QUAD_WORD, 0},	// CR11,
		{Operand::Size::QUAD_WORD, 0},	// CR12,
		{Operand::Size::QUAD_WORD, 0},	// CR13,
		{Operand::Size::QUAD_WORD, 0},	// CR14,
		{Operand::Size::QUAD_WORD, 0}	// CR15,
	};
}
