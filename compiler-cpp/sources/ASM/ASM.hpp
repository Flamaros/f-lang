#pragma once

// This file is the "public" API of the asm module.
// It aims to be used to generate the machine code and the final binary file.
// In a production stage it will provide an API to be use by compiler backend
// (IR to ASM conversion), but without using intermediate text file like almost
// all compilers are doing.
// Here we want to have something much simpler (no ASM file parsing,...).
//
// There will be ASM file loading for development and debugging purpose, but it
// shouldn't be used in production. A way to generate an ASM file from the production
// API wil be given again for development and debugging purpose.

#include <fstd/language/types.hpp>
#include <fstd/language/string_view.hpp>
#include <fstd/system/path.hpp>
#include <fstd/container/array.hpp>
#include <fstd/container/bucket_array.hpp>
#include <fstd/container/hash_table.hpp>
#include <fstd/stream/memory_write_stream.hpp>

// @TODO include the proper version of ASM depending on the targetted architecture
#include "generated/ASM_x64.hpp"

namespace f
{
	namespace ASM
	{
		constexpr uint8_t	NONE = 0b0;
		constexpr size_t	NB_MAX_OPERAND_PER_INSTRUCTION = 4;

		struct Effective_Address
		{
			enum class Scale_Value
			{
				V1	= 0b00,
				V2	= 0b01,
				V4	= 0b10,
				V8	= 0b11,
			};

			// Used only for type EFFECTIVE_ADDRESS
			// SIB byte is 8 upper bits
			// Displacement is on lower bits depending on the size of the Operand
			// @Warning Displacement can be in 64bits in long mode, in this case there is no SIB byte
			// and the Displacement fill completely the value
			// This form exist only for the MOV instruction with AL, AX, EAX, RAX register and the displacement is an absolute address
			// If size of Operand is Size::NONE then the displacement can comes from a label (Address_Flag::FROM_LABEL should be set)
			uint64_t	value;
		};

		struct Operand
		{
			enum Type_Flags : uint8_t
			{
				REGISTER			= 0b0001,
				IMMEDIATE			= 0b0010,	// Labels are immediate address values (Address_Flags::From_LABEL is set in this case)
				IMMEDIATE_SIGNED	= 0b0100,	// Explicitely signed in the ASM source @TODO unecessary?
				ADDRESS				= 0b1000,	// @TOOD may have no sense because identifier are immediate values because we replace it by the value (an address)
			};
			uint8_t	type_flags;	// @Fuck workaround to be able to use the enum as flags without cast

			enum class Size : uint8_t	// @TODO replace that by a size in bytes???
			{
				// @Warning should be from the slowest to the biggest size

				NONE,	// when the operand isn't used
				UNKNOWN,	// For some register not documented and/or I don't plan to use
				BYTE,
				WORD,
				DOUBLE_WORD,
				QUAD_WORD,
				ADDRESS_SIZE	// @Warning can be an issue in 32bits or with SSE instructions, because the order can be wrong
			}				size;

			// @TODO identifier (as string view?)
			// I may have to put function names as label to get addresses
			union Value
			{
				Register                _register;
				int64_t			        integer;
				uint64_t		        unsigned_integer;
				float			        real_32;
				double			        real_64;
				long double		        real_max;

				Effective_Address		EA;
			}				value;

			enum Address_Flags : uint8_t
			{
				UNKNOWN								= 0b0000'0000,
				FROM_LABEL							= 0b0000'0001,
				ABSOLUTE							= 0b0000'0010,	// @Warning RIP relative by default (abs keyword or a register used in EA make the address absolute)
				EFFECTIVE_ADDRESS					= 0b0000'0100,	// @Warning require a ModRM byte and add a SIB byte	- can have the displacement part FROM_LABEL or not (from immediate value)
				EFFECTIVE_ADDRESS_EXTEND_INDEX		= 0b0000'1000,	// @Warning require extended byte in the REX prefix
				EFFECTIVE_ADDRESS_EXTEND_BASE		= 0b0001'0000,	// @Warning require extended byte in the REX prefix
				EFFECTIVE_ADDRESS_DISPLACEMENT8		= 0b0010'0000,	// @Warning May differ than the operand size
				EFFECTIVE_ADDRESS_DISPLACEMENT32	= 0b0100'0000,	// @Warning May differ than the operand size
			};

			fstd::language::string_view	label;	// Only used when Address_Flags::FROM_LABEL is set on address_flags
			uint8_t						address_flags;
		};

		struct Operand_Encoding_Desc
		{
			enum Encoding_Flags : uint8_t
			{
				REGISTER_MODR			= 0b0001,
				REGISTER_ADD_TO_OPCODE	= 0b0010,
				IMMEDIATE_SIGNED		= 0b0100	// This entry support signed integer immediate values
			};

			Operand		op;
			uint8_t		encoding_flags;
		};

		struct Register_Desc
		{
			Operand::Size	size;
			uint8_t			id;
		};

		struct Instruction_Desc
		{
			enum Encoding_Flags : uint8_t
			{
				MODE_32					= 0b0001,
				PREFIX_REX_W			= 0b0010,	// NEED REX.W prefix
			};

			enum Operand_Encoding_Flags : uint8_t
			{
				IN_MODRM_REG			= 0b0000'0001,	// r
				IN_MODRM_RM				= 0b0000'0010,	// m
				IN_VEX_V				= 0b0000'0100,	// v
				IMMEDIATE				= 0b0000'1000,	// i
				IN_IS4_OR_IMZ2_REGISTER	= 0b0001'0000,	// s
				IMPLICIT_OPERAND		= 0b0010'0000,	// -
				IN_MID_INDEX_REGISTER	= 0b0100'0000,	// x
			};

			uint32_t				opcode;
			uint8_t					opcode_size;
			uint8_t					operands_encoding[4];	// Can be overriden by flags in the instruction encoding rules
			uint8_t					encoding_flags;
			uint8_t					modr_value;	// (uint8_t)-1 if not present, (uint8_t)-2 for "/r", else the value n of "/n"
			Operand_Encoding_Desc	op_enc_descs[4];
		};

		struct ADDR_TO_PATCH {
			fstd::language::string_view		label;			// Label to search for and obtain addr (it is a string_view of the label token)
			uint32_t						addr_of_addr;	// Where to apply the addr (relative to the begging of the section)
		};

		struct Section
		{
			fstd::language::string_view	name; // string_view of the first token parsed of this section

			fstd::stream::Memory_Write_Stream	stream_data;
			// @TODO Do I need a Bucket_Array or a Memory_Stream (maybe over the bucket array)?
			// I need something on which I can read/write, seek, get pos
			// I want chunks of memory instead of a full contiguous buffer to avoid issue with memory allocation,...
			// But I don't need to release memory, moving,... auto initialization,...

			fstd::container::Bucket_Array<ADDR_TO_PATCH>	addr_to_patch; // bucket_array because we need fast push_back (allocation of bucket_size) and fast iteration (over arrays)
			uint32_t									position_in_file;
			uint32_t									RVA;
		};

		struct Imported_Function
		{
			fstd::language::string_view	name;
			uint32_t					RVA_of_IAT_entry;
		};

		struct Label
		{
			fstd::language::string_view	label;
			Section*					section;	// @Warning not initialized for an Imported_Function
			Imported_Function*			imported_function;	// If not null the label is already used for an imported function
			uint32_t					RVA;		// Offset in bytes relative to the beggining of the section - @Warning not initialized for an Imported_Function
		};

		struct Imported_Library
		{
			typedef fstd::container::Hash_Table<uint16_t, fstd::language::string_view, Imported_Function*, 32> Function_Hash_Table;

			fstd::language::string_view	name; // string_view of the first token parsed of this library
			Function_Hash_Table			functions;
			uint32_t					IAT_RVA; // "global" RVA to the entry in IAT
		};

		struct ASM
		{
			typedef fstd::container::Hash_Table<uint16_t, fstd::language::string_view, Imported_Library*, 32>	Imported_Library_Hash_Table;
			typedef fstd::container::Hash_Table<uint16_t, fstd::language::string_view, Label*, 32>				Label_Hash_Table;

			Imported_Library_Hash_Table		imported_libraries;
			Label_Hash_Table				labels;
			fstd::container::Array<Section>	sections; // @Speed an Array should be good enough, the number of sections will stay very low and search on less than 10 elements is faster on a array than a hash_table
		};

		struct ASM_Data
		{
			fstd::container::Array<Imported_Library>	imported_libraries; // @TODO use Bucket_Array
			fstd::container::Array<Imported_Function>	imported_functions; // @TODO use Bucket_Array
			fstd::container::Array<Label>				labels;				// @TODO use Bucket_Array
		};

		inline bool match_section(const fstd::language::string_view& name, const Section& section) {
			return fstd::language::are_equals(name, section.name);
		}

		void compile_file(const fstd::system::Path& path, ASM& asm_result);

		// Advanced API
		// You should create a section keep the pointer to fill it in an efficient way with function helpers when possible
		// Try to generate valid ASM code else compiler errors may happens in these functions, especially in push_instruction which
		// may fail to find the given instruction for the current targetted architecture
		Section*	create_section(ASM& asm_result, fstd::language::string_view name);
		void		create_Effective_Address(Register base, Register index, Effective_Address::Scale_Value scale, Operand& operand);	// Helper method that generate an Operand with an Effective Address
		bool		push_instruction(Section* section, Instruction instruction, const Operand operands[NB_MAX_OPERAND_PER_INSTRUCTION]);
		void		push_raw_data(Section* section, uint8_t* data, uint32_t size);

		Section*	get_section(const ASM& asm_result, fstd::language::string_view name);

		// @TODO do the production API 
		//   - create the import module

		void debug_print(const ASM& asm_result);

		// Tests

		void test_x64_encoding();
	}
}
