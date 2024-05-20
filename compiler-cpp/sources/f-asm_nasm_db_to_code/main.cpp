#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <iterator>
#include <regex>
#include <vector>
#include <algorithm>
#include <cassert>
#include <unordered_map>

#include <tracy/Tracy.hpp>

using namespace std;

#if defined(TRACY_ENABLE)
void* operator new(std::size_t count)
{
	auto ptr = malloc(count);
	TracyAlloc(ptr, count);
	return ptr;
}
void operator delete(void* ptr) noexcept
{
	TracyFree(ptr);
	free(ptr);
}
#endif

enum class Instruction_Kind_State
{
	UNKNOWN,
	SPECIAL_INSTRUCTIONS,
	CONVENTIONAL_INSTRUCTIONS
};

std::vector<std::string> split(const std::string& s, char delim)
{
	std::vector<std::string> result;
	std::stringstream ss(s);
	std::string item;

	while (getline(ss, item, delim)) {
		result.push_back(item);
	}

	return result;
}

string to_upper(const string& s)
{
	string result = s;
	transform(result.begin(), result.end(), result.begin(), ::toupper);
	return result;
}

static vector<string> x64_instruction_sets = {
	"186",
	"286",
	"386",
	"486",
	"8086",
	"X64",

	// @TODO (Need to support up to 4 operands)
/*
	"3DNOW",
	"MMX",
	"SSE",
	"SSE2",
	"SSE3",
//	"SSE4A",	// AMD only?
	"SSE41",
	"SSE42",
	"SSE5",
	"AVX",
	"AVX2",
	"AVX512",
*/
};

static unordered_map<string, string> register_size_from_disassembler_class = {
	{"reg8", "BYTE"},
	{"reg8_rex", "BYTE"},
	{"reg16", "WORD"},
	{"reg32", "DOUBLE_WORD"},
	{"reg64", "QUAD_WORD"},

	{"sreg", "WORD"},	// In x86 (in real and protected mode)

	{"creg", "ADDRESS_SIZE"},	// 32bits en x86 - 64bits In x64 in long mode

	{"dreg", "ADDRESS_SIZE"},	// 32bits en x86 - 64bits In x64 in long mode

	{"treg", "UNKNOWN"},
	{"fpureg", "UNKNOWN"},
	{"mmxreg", "UNKNOWN"},
	{"xmmreg", "UNKNOWN"},
	{"ymmreg", "UNKNOWN"},
	{"zmmreg", "UNKNOWN"},
	{"opmaskreg", "UNKNOWN"},
	{"bndreg", "UNKNOWN"},
};

struct Operand_Info
{
	string	type;
	string	size;
};

unordered_map<string, Operand_Info> operands_info;

void generate_operands_info()
{
	operands_info = {
	{"imm", {"Operand::Type_Flags::IMMEDIATE", "QUAD_WORD"}},
	{"imm8", {"Operand::Type_Flags::IMMEDIATE", "BYTE"}},
	{"imm16", {"Operand::Type_Flags::IMMEDIATE", "WORD"}},
	{"imm32", {"Operand::Type_Flags::IMMEDIATE", "DOUBLE_WORD"}},
	{"imm64", {"Operand::Type_Flags::IMMEDIATE", "QUAD_WORD"}},

	// @TODO do all size of mem
	{"mem", {"Operand::Type_Flags::ADDRESS", "ADDRESS_SIZE"}},

	{"reg", {"Operand::Type_Flags::REGISTER", "QUAD_WORD"}},
	{"reg8", {"Operand::Type_Flags::REGISTER", "BYTE"}},
	{"reg16", {"Operand::Type_Flags::REGISTER", "WORD"}},
	{"reg32", {"Operand::Type_Flags::REGISTER", "DOUBLE_WORD"}},
	{"reg64", {"Operand::Type_Flags::REGISTER", "QUAD_WORD"}},

	// @TODO what rm stand for exactly, register and memory or RM of ModRM ?
	{"rm16", {"Operand::Type_Flags::REGISTER | Operand::Type_Flags::ADDRESS", "WORD"}},
	{"rm32", {"Operand::Type_Flags::REGISTER | Operand::Type_Flags::ADDRESS", "DOUBLE_WORD"}},
	{"rm64", {"Operand::Type_Flags::REGISTER | Operand::Type_Flags::ADDRESS", "QUAD_WORD"}},

	// @TODO add registers specified by name
	// @TODO sbyteword,...
	};

	// @TODO get registers to add them to operands_info (some instructions support only few specific registers)
	// Need to get registers by parameter
}

/// Instruction encoding rules (comments from insns.pl file of NASM compiler)
/// [operands: opcodes]
///
/// The operands word lists the order of the operands :
///
/// r = register field in the modr / m
/// m = modr / m
/// v = VEX "v" field
/// i = immediate
/// s = register field of is4 / imz2 field
/// - = implicit (unencoded) operand
/// x = indeX register of mib. 014..017 bytecodes are used.
///
/// For an operand that should be filled into more than one field,
/// enter it as e.g. "r+v".
std::string operands_regex_string = R"REG(([\w\-]{0,4}:?\s+).*)REG";

///my% plain_codes = (
///		'o16' = > 0320, # 16 - bit operand size
///		'o32' = > 0321, # 32 - bit operand size
///		'odf' = > 0322, # Operand size is default
///		'o64' = > 0324, # 64 - bit operand size requiring REX.W
///		'o64nw' = > 0323, # Implied 64 - bit operand size(no REX.W)
///		'a16' = > 0310,
///		'a32' = > 0311,
///		'adf' = > 0312, # Address size is default
///		'a64' = > 0313,
///		'!osp' = > 0364,
///		'!asp' = > 0365,
///		'f2i' = > 0332, # F2 prefix, but 66 for operand size is OK
///		'f3i' = > 0333, # F3 prefix, but 66 for operand size is OK
///		'mustrep' = > 0336,
///		'mustrepne' = > 0337,
///		'rex.l' = > 0334,
///		'norexb' = > 0314,
///		'norexx' = > 0315,
///		'norexr' = > 0316,
///		'norexw' = > 0317,
///		'repe' = > 0335,
///		'nohi' = > 0325, # Use spl / bpl / sil / dil even without REX
///		'nof3' = > 0326, # No REP 0xF3 prefix permitted
///		'norep' = > 0331, # No REP prefix permitted
///		'wait' = > 0341, # Needs a wait prefix
///		'resb' = > 0340,
///		'np' = > 0360, # No prefix
///		'jcc8' = > 0370, # Match only if Jcc possible with single byte
///		'jmp8' = > 0371, # Match only if JMP possible with single byte
///		'jlen' = > 0373, # Length of jump
///		'hlexr' = > 0271,
///		'hlenl' = > 0272,
///		'hle' = > 0273,
///
///		# This instruction takes XMM VSIB
///		'vsibx' = > 0374,
///		'vm32x' = > 0374,
///		'vm64x' = > 0374,
///
///		# This instruction takes YMM VSIB
///		'vsiby' = > 0375,
///		'vm32y' = > 0375,
///		'vm64y' = > 0375,
///
///		# This instruction takes ZMM VSIB
///		'vsibz' = > 0376,
///		'vm32z' = > 0376,
///		'vm64z' = > 0376,
///		);
std::string opcode_flags_regex_string = R"REG((\s*)(o16|o32|odf|o64|o64nw|a16|a32|adf|a64|\!osp|\!asp|f2i|f3i|mustrep|mustrepne|rex\.l|norexb|norexx|norexr|norexw|repe|nohi|nof3|norep|wait|resb|np|jcc8|jmp8|jlen|hlexr|hlenl|hle|vsibx|vm32x|vm64x|vsiby|vm32y|vm64y|vsibz|vm32z|vm64z)\s+.*)REG";

std::string opcode_regex_string = R"REG((\s*)([0-9a-f]{2})\s*.*)REG";
std::string modr_value_regex_string = R"REG((\/[\dr]\s+)*.*)REG";

/// my% imm_codes = (
/// 	'ib' = > 020, # imm8
/// 	'ib,u' = > 024, # Unsigned imm8
/// 	'iw' = > 030, # imm16
/// 	'ib,s' = > 0274, # imm8 sign - extended to opsize or bits
/// 	'iwd' = > 034, # imm16 or imm32, depending on opsize
/// 	'id' = > 040, # imm32
/// 	'id,s' = > 0254, # imm32 sign - extended to 64 bits
/// 	'iwdq' = > 044, # imm16 / 32 / 64, depending on addrsize
/// 	'rel8' = > 050,
/// 	'iq' = > 054,
/// 	'rel16' = > 060,
/// 	'rel' = > 064, # 16 or 32 bit relative operand
/// 	'rel32' = > 070,
/// 	'seg' = > 074,
/// 	);
std::string immediates_regex_string = R"REG((ib|ib,u|iw|ib,s|iwd|id|id,s|iwdq|rel8|iq|rel16|rel|rel32|seg)?)REG";

int main(int ac, char** av)
{
#if defined(TRACY_ENABLE)
	tracy::SetThreadName("Main thread");
#endif
	FrameMark;

	filesystem::create_directories("./sources/ASM/generated");

	ofstream	asm_x64_cpp_file;
	ofstream	asm_x64_hpp_file;

	asm_x64_cpp_file.open("./sources/ASM/generated/ASM_x64.cpp", ios::trunc | ios::binary);
	asm_x64_hpp_file.open("./sources/ASM/generated/ASM_x64.hpp", ios::trunc | ios::binary);

	asm_x64_cpp_file <<
R"CODE(// This file is generated by f-asm_nasm_db_to_code program

#include "ASM_x64.hpp"

#include "../ASM.hpp"

namespace f::ASM
{
)CODE";

	asm_x64_hpp_file <<
R"CODE(// This file is generated by f-asm_nasm_db_to_code program

#pragma once

#include <fstd/language/types.hpp>

namespace f::ASM
{
)CODE";

	stringstream x64_cpp_instruction_desc_table_indices;
	stringstream x64_cpp_instruction_desc_table;
	stringstream x64_cpp_register_desc_table;

	x64_cpp_instruction_desc_table_indices <<
R"CODE(    size_t g_instruction_desc_table_indices[(size_t)Instruction::COUNT + 1] = {
        0,	// UNKNOWN
)CODE";

	x64_cpp_instruction_desc_table <<
R"CODE(    Instruction_Desc g_instruction_desc_table[] = {
        // UNKNOWN
)CODE";

	x64_cpp_register_desc_table <<
R"CODE(    Register_Desc	g_register_desc_table[(size_t)Register::COUNT] = {
        {Operand::Size::NONE, 0},	// UNKNOWN,

)CODE";

	stringstream x64_hpp_instruction_enum;
	stringstream x64_hpp_register_enum;

	x64_hpp_instruction_enum <<
R"CODE(    enum class Instruction : uint16_t
    {
        UNKNOWN,

)CODE";

	x64_hpp_register_enum <<
R"CODE(    enum class Register : uint8_t // @TODO Can we have more than 256 registers
    {
        UNKNOWN,

)CODE";

	string		read_line;

	ifstream	regs_dat_file("./data/regs.dat");
	if (regs_dat_file.is_open())
	{
		regex	register_desc_regex(R"REG(([\w-]+)\s+(\w+)\s+([\w,]+)\s+(\d+)\s*(\w*))REG");
		smatch	register_desc_match_result;
		regex	registers_regex(R"REG(([a-z]+)(\d+)-(\d+)([a-z]*))REG");
		smatch	registers_match_result;

		size_t current_line = 0; // For debugging

		while (getline(regs_dat_file, read_line))
		{
			current_line++;

			if (read_line.starts_with("##")) { // Verbose comment
				continue;
			}
			
			if (read_line.starts_with("#")) {
				continue;
			}

			if (regex_match(read_line, register_desc_match_result, register_desc_regex)) {
				string registers = register_desc_match_result[1];
				string assembler_class = register_desc_match_result[2];
				string disassembler_classes_string = register_desc_match_result[3];
				string register_number = register_desc_match_result[4];
				string token_flag = register_desc_match_result[4];

				vector<string> disassembler_classes = split(disassembler_classes_string, ',');
				string register_size = register_size_from_disassembler_class[disassembler_classes[0]];

				if (regex_match(registers, registers_match_result, registers_regex)) {
					string prefix = registers_match_result[1];
					string first_index = registers_match_result[2];
					string last_index = registers_match_result[3];
					string suffix = registers_match_result[4];

					int first = stoi(first_index);
					int last = stoi(last_index);
					for (int i = first; i <= last; i++) {
						x64_hpp_register_enum << "        " << to_upper(prefix) << i << to_upper(suffix) << "," << endl;
						x64_cpp_register_desc_table << "        " << "{Operand::Size::" << register_size << ", " << stoi(register_number) << "}," << endl;
					}
				}
				else {
					x64_hpp_register_enum << "        " << to_upper(registers) << "," << endl;
					x64_cpp_register_desc_table << "        " << "{Operand::Size::" << register_size << ", " << stoi(register_number) << "}," << endl;
				}
			}


			// @TODO
			// Remplir la fonction de remplissage de la hash_table des register en lower et upper case (optionnel avec un define?)

		}
	}

	generate_operands_info();

	ifstream	insns_dat_file("./data/insns.dat");
	if (insns_dat_file.is_open())
	{
		regex	instruction_desc_regex(R"REG((\w+)\s+([\w,\|\:]+)\s+\[([\w\s\/\-:,]+)\]\s+([\w,]+))REG");
		smatch	instruction_desc_match_result;
		regex	operands_regex(operands_regex_string);
		smatch	operands_match_result;
		regex	opcode_flags_regex(opcode_flags_regex_string);
		smatch	opcode_flags_match_result;
		regex	opcode_regex(opcode_regex_string);
		smatch	opcode_match_result;
		regex	modr_value_regex(modr_value_regex_string);
		smatch	modr_value_match_result;
		regex	immediates_regex(immediates_regex_string);
		smatch	immediates_match_result;

		string					previous_instruction_name;
		size_t					x64_current_instruction_index = 0;
		size_t					x64_nb_instruction = 0;
		Instruction_Kind_State	instruction_state = Instruction_Kind_State::UNKNOWN;

		size_t current_line = 0; // For debugging
		while (getline(insns_dat_file, read_line))
		{
			current_line++;

			if (read_line.starts_with(";;")) { // Verbose comment
				continue;
			}

			if (read_line.starts_with(";# Special instructions (pseudo-ops)")) { // Special instructions delimiter comment
				instruction_state = Instruction_Kind_State::SPECIAL_INSTRUCTIONS;
				continue;
			}
			else if (read_line.starts_with(";# Conventional instructions")) { // Conventional instructions delimiter comment
				instruction_state = Instruction_Kind_State::CONVENTIONAL_INSTRUCTIONS;
				continue;
			}

			if (instruction_state != Instruction_Kind_State::CONVENTIONAL_INSTRUCTIONS) {
				continue;
			}

			if (read_line.starts_with(";#")) { // Special comment
				// @TODO handle those particular instruction set types (AVX2, vendor specific, SSE5,...)

				//read_line.replace(0, 2, "//");
				//x64_cpp_instruction_desc_table << read_line << endl;
				continue;
			}
			if (read_line.starts_with(';')) { // Normal comment
				read_line.replace(0, 1, "//");
				x64_cpp_instruction_desc_table << "        " << read_line << endl;
				x64_hpp_instruction_enum << "        " << read_line << endl;
				continue;
			}

			if (regex_match(read_line, instruction_desc_match_result, instruction_desc_regex)) {
				sub_match instruction = instruction_desc_match_result[1];
				sub_match operand_types = instruction_desc_match_result[2];
				sub_match encoding_rules = instruction_desc_match_result[3];
				sub_match architectures_flags = instruction_desc_match_result[4];

				bool	is_x64_supported = false;

				vector<string>	architectures_flags_array = split(architectures_flags, ',');
				for (const string& architecture_or_flag : architectures_flags_array) {
					if (!is_x64_supported && find(x64_instruction_sets.begin(), x64_instruction_sets.end(), architecture_or_flag) != x64_instruction_sets.end()) {
							is_x64_supported = true;
					}
				}

				// @TODO skip instruction not supported by the targetted architecture (x64)
				// @TODO future when multiple architectures are supported:
				//   - create a bool by architecture to tell if the instruction is supported by the architecture
				//   - put if on the bool to write in the streams of architectures

				if (instruction != previous_instruction_name) {
					if (is_x64_supported) {
						x64_cpp_instruction_desc_table_indices << "        "
							<< x64_current_instruction_index << "," << "\t// " << instruction << endl;

						x64_hpp_instruction_enum << "        "
							<< instruction << ",\t// " << x64_nb_instruction + 1 << endl;

						x64_cpp_instruction_desc_table << endl << "        " << "// " << instruction << endl;

						x64_nb_instruction++;
					}
				}

				if (is_x64_supported) {
					std::string encoding_rules_string = encoding_rules.str();

					std::string opcode;
					uint8_t		opcode_size = 0;
					std::vector<std::string> operand_descs;

#if defined(_DEBUG)
					if (encoding_rules_string.find("wait") != std::string::npos)
					{
						int i = 1;
					}
#endif

					if (regex_match(encoding_rules_string, operands_match_result, operands_regex)
						&& operands_match_result[1].length()) {

						encoding_rules_string.erase(0, operands_match_result[1].length());
					}

					// operands
					vector<string>	operands_types_array = split(operand_types, ',');
					for (const string& operand : operands_types_array) {
						stringstream	operand_desc;

						if (operand == "void") {	// Here just the format of the string that should contains "4 columns"
							break;
						}

						auto it_operand_info = operands_info.find(operand);
						if (it_operand_info == operands_info.end()) {
//							operand_desc << "0xBAD'F00D /* Operand not correctly analyzed */";
//							operand_descs.push_back(operand_desc.str());
						}
						else {
							operand_desc << "{" << it_operand_info->second.type << ", " << "Operand::Size::" << it_operand_info->second.size << "}";
							operand_descs.push_back(operand_desc.str());
						}
					}

					while (regex_match(encoding_rules_string, opcode_flags_match_result, opcode_flags_regex)
						&& opcode_flags_match_result[2].length()) {

						encoding_rules_string.erase(0, opcode_flags_match_result[1].length());
						encoding_rules_string.erase(0, opcode_flags_match_result[2].length());
					}

					while (regex_match(encoding_rules_string, opcode_match_result, opcode_regex)
						&& opcode_match_result[2].length()) {
						opcode += opcode_match_result[2];
						opcode_size += 1;

						encoding_rules_string.erase(0, opcode_match_result[1].length());
						encoding_rules_string.erase(0, opcode_match_result[2].length());
					}

					assert(!opcode.empty());

					{	// write instruction line in x64_cpp_instruction_desc_table
						x64_cpp_instruction_desc_table << "        "
							<< "{0x" << opcode << ", " << std::to_string(opcode_size);
						for (const string& operand_desc : operand_descs) {
							x64_cpp_instruction_desc_table << ", " << operand_desc;
						}
						x64_cpp_instruction_desc_table << "},"
							<< " // " << read_line
							<< endl;
					}

					x64_current_instruction_index++;
				}

				previous_instruction_name = instruction;
			}
		}
	}

	x64_cpp_instruction_desc_table_indices << "    };" << endl << endl;
	x64_cpp_instruction_desc_table << "    };" << endl << endl;
	x64_cpp_register_desc_table << "    };" << endl << endl;

	asm_x64_cpp_file << x64_cpp_instruction_desc_table_indices.str() << endl;
	asm_x64_cpp_file << x64_cpp_instruction_desc_table.str() << endl;
	asm_x64_cpp_file << x64_cpp_register_desc_table.str() << endl;
	asm_x64_cpp_file << "}" << endl;

	x64_hpp_instruction_enum <<
R"CODE(
		COUNT,
    };
)CODE";

	x64_hpp_register_enum <<
R"CODE(
		COUNT,
    };
)CODE";

	asm_x64_hpp_file << x64_hpp_instruction_enum.str() << endl;
	asm_x64_hpp_file << x64_hpp_register_enum.str() << endl;
	asm_x64_hpp_file << "}" << endl;

	FrameMark;
	return 0;
}
