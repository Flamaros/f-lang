#include "ASM.hpp"

#include "ASM_lexer.hpp"
#include "globals.hpp"

#include <fstd/language/defer.hpp>
#include <fstd/language/flags.hpp>
#include <fstd/memory/array.hpp>
#include <fstd/system/file.hpp>
#include <fstd/stream/array_read_stream.hpp>

#include <third-party/SpookyV2.h>

using namespace fstd;

#define SECTION_NAME_MAX_LENGTH	8	// 8 is the maximum supported by PE file format

#define NB_PREALLOCATED_IMPORTED_LIBRARIES				128
#define NB_PREALLOCATED_IMPORTED_FUNCTIONS_PER_LIBRARY	4096
#define NB_PREALLOCALED_SECTIONS						5		// .text, .rdata, .data, .reloc, .idata
#define NB_LABELS_PER_TOKEN								1 / 10.0				

namespace f::ASM
{
	// Separated on two tables because in C++ the second dimension of the array can't use an unknown size
	// So we have a table of indices, and getting the indice for the following instruction can be used to compute the number of descs
	// for the current instruction. That is why I have one more indice than the instruction count for the last instruction.
	extern size_t g_instruction_desc_table_indices[(size_t)Instruction::COUNT + 1];
	extern Instruction_Desc g_instruction_desc_table[];
	extern Register_Desc g_register_desc_table[(size_t)Register::COUNT];

	inline Imported_Library* allocate_imported_library()
	{
		// Ensure that no reallocation could happen during the resize
		core::Assert(memory::get_array_size(globals.asm_data.imported_libraries) < memory::get_array_reserved(globals.asm_data.imported_libraries));

		memory::resize_array(globals.asm_data.imported_libraries, memory::get_array_size(globals.asm_data.imported_libraries) + 1);

		Imported_Library* new_imported_library = memory::get_array_last_element(globals.asm_data.imported_libraries);
		return new_imported_library;
	}

	inline Imported_Function* allocate_imported_function()
	{
		// Ensure that no reallocation could happen during the resize
		core::Assert(memory::get_array_size(globals.asm_data.imported_functions) < memory::get_array_reserved(globals.asm_data.imported_functions));

		memory::resize_array(globals.asm_data.imported_functions, memory::get_array_size(globals.asm_data.imported_functions) + 1);

		Imported_Function* new_imported_function = memory::get_array_last_element(globals.asm_data.imported_functions);
		return new_imported_function;
	}

	inline Label* allocate_Label()
	{
		// Ensure that no reallocation could happen during the resize
		core::Assert(memory::get_array_size(globals.asm_data.labels) < memory::get_array_reserved(globals.asm_data.labels));

		memory::resize_array(globals.asm_data.labels, memory::get_array_size(globals.asm_data.labels) + 1);

		Label* new_label = memory::get_array_last_element(globals.asm_data.labels);
		return new_label;
	}

	void compile(ASM& asm_result, stream::Array_Read_Stream<Token>& stream);

	void compile_file(const fstd::system::Path& path, const fstd::system::Path& output_path, bool shared_library, ASM& asm_result)
	{
		ZoneScopedNC("f::ASM::compile_file", 0x1b5e20);

		fstd::memory::Array<Token>	tokens;

		initialize_lexer();
		lex(path, tokens);

#if defined(FSTD_DEBUG)
		print(tokens);
#endif

		// Result data
		{
			memory::hash_table_init(asm_result.imported_libraries, &language::are_equals);
			memory::hash_table_init(asm_result.labels, &language::are_equals);

			memory::init(asm_result.sections);
			memory::reserve_array(asm_result.sections, NB_PREALLOCALED_SECTIONS);
		}

		// Working data (stored in globals)
		{
			memory::init(globals.asm_data.imported_libraries);
			memory::reserve_array(globals.asm_data.imported_libraries, NB_PREALLOCATED_IMPORTED_LIBRARIES);

			memory::init(globals.asm_data.imported_functions);
			memory::reserve_array(globals.asm_data.imported_functions, NB_PREALLOCATED_IMPORTED_LIBRARIES * NB_PREALLOCATED_IMPORTED_FUNCTIONS_PER_LIBRARY);

			memory::init(globals.asm_data.labels);
			memory::reserve_array(globals.asm_data.labels, (size_t)(memory::get_array_size(tokens) * NB_LABELS_PER_TOKEN) + 1);	// + 1 to ceil the value
		}

		stream::Array_Read_Stream<Token>	stream;
		stream::init<Token>(stream, tokens);

		compile(asm_result, stream);

		// @TODO @Speed
		// Il faut preallouer le buffer dans lequel je vais mettre le code, il faut compter 1 instructions par ligne dans le ficher.
		// Et 15bytes par instructions.

		// I may have to use '.' character do allow user type specify the type he want to use (like a cast)
		// "mov.d" will force the code generator to use a 32 bits version of "mov" instruction (d meaning dword).

		// @TODO
		//
		// Je n'ai pas besoin de g�n�rer un AST
		//   - Il faut simplement remplir des structures qui correspondent aux diff�rentes parties du binaire
		//     par exemple il faut pouvoir fragmenter la d�claration des imports de modules, pareil pour les sections
		//   - Il faut sans doute une table de label par section
		//     un label n'est qu'une key dans une table de hash indiquant l'offset par rapport au d�but de la section
		//	   afin de pouvoir utiliser un label avant sa d�claration il faut avoir une liste des addresse � patcher
		//	   struct ADDR_TO_PATCH {
		//			string	label;			// Label to search for and obtain addr
		//			u32		addr_of_addr;	// Where to apply the addr
		//     };
		//
		// Il doit y avoir un label main dans la section .text
		//    - Sinon erreur
		//
		//
		// BONUS:
		//		Un label est �quivalent � une addresse, si j'ai un symbole comme $ pour r�cup�rer l'addresse de l'instruction
		//		actuelle, je dois pouvoir impl�menter des op�rations simple sans AST et sans gestion de pr�c�dence sur les op�rateurs.
		//		Car le but est de pouvoir �ventuellement faire:
		//			message:	db "Hello World"		// message est un label qui a une addresse
		//			len:		dd	$ - message			// $ est l'addresse de l'instruction courante
		//		Dans l'exemple de code pr�c�dent je n'ai absolument pas besoin de me soucier de la pr�c�dence des op�rateurs
		//		Car l'op�ration est trop simple, je ne suis pas sur d'avoir besoin de plus pour la section .data
		//		Et encore comme mon ASM vise � �tre g�n�r�, normalement le front-end de mon language n'aura m�me pas
		//		besoin de cette fonctionnalit�, c'est pour �a que c'est du pure bonus pour mes dev.
	}

	void parse_module(ASM& asm_result, stream::Array_Read_Stream<Token>& stream)
	{
		ZoneScopedNC("f::ASM::parse_module", 0x1b5e20);

		Token	current_token;
		Token	module_name;

		stream::peek<Token>(stream);	// MODULE

		module_name = stream::get(stream);
		if (module_name.type != Token_Type::STRING_LITERAL) {
			report_error(Compiler_Error::error, module_name, "Missing string after \"MODULE\" keyword, you should specify the name of the module.");
		}

		// Register the module
		Imported_Library** found_imported_lib;

		uint64_t lib_hash = SpookyHash::Hash64((const void*)fstd::language::to_utf8(module_name.text), fstd::language::get_string_size(module_name.text), 0);
		uint16_t lib_short_hash = lib_hash & 0xffff;

		found_imported_lib = fstd::memory::hash_table_get(asm_result.imported_libraries, lib_short_hash, module_name.text);
		if (found_imported_lib == nullptr) {
			Imported_Library* new_imported_lib = allocate_imported_library();

			new_imported_lib->name = module_name.text;
			fstd::memory::hash_table_init(new_imported_lib->functions, &fstd::language::are_equals);

			found_imported_lib = fstd::memory::hash_table_insert(asm_result.imported_libraries, lib_short_hash, module_name.text, new_imported_lib);
		}
		// --

		stream::peek<Token>(stream);	// module name

		current_token = stream::get(stream);
		if (current_token.type != Token_Type::SYNTAXE_OPERATOR
			&& current_token.value.punctuation != Punctuation::OPEN_BRACE) {
			report_error(Compiler_Error::error, current_token, "Missing '{' character after \"IMPORTS\" keyword!");
		}

		stream::peek<Token>(stream);	// {

		while (stream::is_eof(stream) == false)
		{
			current_token = stream::get(stream);

			if (current_token.type == Token_Type::SYNTAXE_OPERATOR
				&& current_token.value.punctuation == Punctuation::CLOSE_BRACE) {
				stream::peek<Token>(stream);	// }
				return;
			}

			if (current_token.type != Token_Type::IDENTIFIER) {
				report_error(Compiler_Error::error, module_name, "Only identifiers are supported inside the \"MODULE\" scope. Those identifier specifies the function names to import.");
			}

			// Register the function
			Imported_Function** found_imported_func;

			uint64_t func_hash = SpookyHash::Hash64((const void*)fstd::language::to_utf8(current_token.text), fstd::language::get_string_size(current_token.text), 0);
			uint16_t func_short_hash = func_hash & 0xffff;

			found_imported_func = fstd::memory::hash_table_get((*found_imported_lib)->functions, func_short_hash, module_name.text);

			if (found_imported_func == nullptr) {
				Imported_Function* new_imported_func = allocate_imported_function();

				new_imported_func->name = current_token.text;
				new_imported_func->name_RVA = 0;

				fstd::memory::hash_table_insert((*found_imported_lib)->functions, func_short_hash, module_name.text, new_imported_func);
			}
			else {
				report_error(Compiler_Error::warning, current_token, "Function already imported!"); // @TODO for the current module
			}
			// --

			stream::peek<Token>(stream); // function name
		}

		report_error(Compiler_Error::error, current_token, "End of file reached. Missing '}' to delimite the \"MODULE\" scope.");
	}

	void parse_import(ASM& asm_result, stream::Array_Read_Stream<Token>& stream)
	{
		ZoneScopedNC("f::ASM::parse_import", 0x1b5e20);

		Token	current_token;

		stream::peek<Token>(stream);	// IMPORTS

		current_token = stream::get(stream);
		if (current_token.type != Token_Type::SYNTAXE_OPERATOR
			&& current_token.value.punctuation != Punctuation::OPEN_BRACE) {
			report_error(Compiler_Error::error, current_token, "Missing '{' character after \"IMPORTS\" keyword!");
		}

		stream::peek<Token>(stream);	// {

		while (stream::is_eof(stream) == false)
		{
			current_token = stream::get(stream);

			if (current_token.type == Token_Type::KEYWORD) {
				if (current_token.value.keyword == Keyword::MODULE) {
					parse_module(asm_result, stream);
				}
				else {
					report_error(Compiler_Error::error, current_token, "Unexpected keyword in \"IMPORTS\" scope.");
				}
			}
			else if (current_token.type == Token_Type::SYNTAXE_OPERATOR
				&& current_token.value.punctuation == Punctuation::CLOSE_BRACE) {
				stream::peek<Token>(stream);	// }
				return;
			}
			else {
				report_error(Compiler_Error::error, current_token, "Unexpected token in \"IMPORTS\" scope. You should put \"MODULE\" scopes here.");
			}
		}

		report_error(Compiler_Error::error, current_token, "End of file reached. Missing '}' to delimite the \"IMPORTS\" scope.");
	}

	void parse_pseudo_instruction(ASM& asm_result, stream::Array_Read_Stream<Token>& stream, Section* section)
	{
		ZoneScopedNC("f::ASM::parse_pseudo_instruction", 0x1b5e20);

		size_t	starting_line;
		Token	current_token;

		current_token = stream::get(stream);
		starting_line = current_token.line;
		fstd::core::Assert(
			current_token.type == Token_Type::KEYWORD
			&& current_token.value.keyword >= Keyword::DB
			&& current_token.value.keyword < Keyword::COUNT);

		if (current_token.value.keyword == Keyword::DB) { // @TODO put it in a specific function?
			stream::peek(stream);	// db
			while (current_token.line == starting_line && stream::is_eof(stream) == false)
			{
				if (current_token.type == Token_Type::STRING_LITERAL
					|| current_token.type == Token_Type::STRING_LITERAL_RAW) {
					push_raw_data(section, to_utf8(*current_token.value.string), (uint32_t)get_string_size(*current_token.value.string));
					stream::peek(stream); // String literal
				}
				else if (current_token.type == Token_Type::NUMERIC_LITERAL_I8) {
					unsigned char value = (unsigned char)current_token.value.integer;
					push_raw_data(section, &value, sizeof(value));	// byte value
					stream::peek(stream);
				}
				else if (current_token.type == Token_Type::NUMERIC_LITERAL_UI8) {
					unsigned char value = (unsigned char)current_token.value.integer;
					push_raw_data(section, &value, sizeof(value));	// byte value
					stream::peek(stream);
				}
				else if (current_token.type == Token_Type::SYNTAXE_OPERATOR
					&& current_token.value.punctuation == Punctuation::COMMA) {
					stream::peek(stream);	// ,
				}
				else if (is_numeric_litral(current_token.type)) {
					report_error(Compiler_Error::error, current_token, "Operand value out of range: if you specify a character by its value it should be in range [0-256].");
				}

				current_token = stream::get(stream);
			}
			if (stream::is_eof(stream)) {
				// @TODO Test and improve this error message
				report_error(Compiler_Error::error, current_token, "End of file reached before the closure of the section");
			}
		}
		else {
			// @TODO unknown pseudo instruction
			report_error(Compiler_Error::error, current_token, "Unknown pseudo instruction");
		}
	}

	void parse_instruction(ASM& asm_result, stream::Array_Read_Stream<Token>& stream, Section* section)
	{
		ZoneScopedNC("f::ASM::parse_instruction", 0x1b5e20);

		size_t		starting_line;
		Token		current_token;
		size_t		operand_index = 0;
		Operand		operands[NB_MAX_OPERAND_PER_INSTRUCTION] = {};	// @Warning Initialized to 0 if one operand isn't used (else the search may fails)
		Instruction	instruction;
		Token		instruction_token;

		current_token = stream::get(stream);
		starting_line = current_token.line;
		fstd::core::Assert(
			current_token.type == Token_Type::INSTRUCTION
			&& current_token.value.instruction >= Instruction::ADD
			&& current_token.value.instruction < Instruction::COUNT);

		while (current_token.line == starting_line && stream::is_eof(stream) == false)
		{
			if (current_token.type == Token_Type::SYNTAXE_OPERATOR
				&& current_token.value.punctuation == Punctuation::COMMA) {
				operand_index++;

				stream::peek(stream); // ,
			}
			else if (current_token.type == Token_Type::INSTRUCTION)
			{
				instruction_token = current_token;
				instruction = current_token.value.instruction;

				stream::peek(stream); // Instruction
			}
			else {
				if (operand_index >= NB_MAX_OPERAND_PER_INSTRUCTION) {
					report_error(Compiler_Error::error, current_token, "It seems that you are trying to use more operands than instructions can support.");
				}

				if (current_token.type == Token_Type::REGISTER) {
					operands[operand_index].type_flags = Operand::Type_Flags::REGISTER;
					operands[operand_index].value._register = current_token.value._register;
					operands[operand_index].size = g_register_desc_table[(size_t)current_token.value._register].size;

					stream::peek(stream); // Register
				}
				else if (current_token.type == Token_Type::IDENTIFIER) {
					operands[operand_index].type_flags = Operand::Type_Flags::ADDRESS;	// @TODO check if all identifier are immediate addresses
//					operands[operand_index].value.integer = current_token.value._register; // @TODO push label or function name here
					operands[operand_index].size = Operand::Size::QUAD_WORD;	// @TODO base size of the target architecture

					// @TODO

					stream::peek(stream); // Identifier
				}
				// @TODO Do we need to do something cleaver to factorize code here?
				// In the lexer, by having the size and sign as flags on numeric literal?
				// Or here with a table that give the type of the numeric literal kind?
				else if (current_token.type == Token_Type::NUMERIC_LITERAL_I8) {
					operands[operand_index].type_flags = Operand::Type_Flags::IMMEDIATE;
					operands[operand_index].size = Operand::Size::BYTE;
					operands[operand_index].value.integer = current_token.value.integer;

					stream::peek(stream); // Numeric value
				}
				else if (current_token.type == Token_Type::NUMERIC_LITERAL_I16) {
					operands[operand_index].type_flags = Operand::Type_Flags::IMMEDIATE;
					operands[operand_index].size = Operand::Size::WORD;
					operands[operand_index].value.integer = current_token.value.integer;

					stream::peek(stream); // Numeric value
				}
				else if (current_token.type == Token_Type::NUMERIC_LITERAL_I32) {
					operands[operand_index].type_flags = Operand::Type_Flags::IMMEDIATE;
					operands[operand_index].size = Operand::Size::DOUBLE_WORD;
					operands[operand_index].value.integer = current_token.value.integer;

					stream::peek(stream); // Numeric value
				}
				else if (current_token.type == Token_Type::NUMERIC_LITERAL_I64) {
					operands[operand_index].type_flags = Operand::Type_Flags::IMMEDIATE;
					operands[operand_index].size = Operand::Size::QUAD_WORD;
					operands[operand_index].value.integer = current_token.value.integer;

					stream::peek(stream); // Numeric value
				}
				else if (current_token.type == Token_Type::NUMERIC_LITERAL_UI8) {
					operands[operand_index].type_flags = Operand::Type_Flags::IMMEDIATE;
					operands[operand_index].size = Operand::Size::BYTE;

					operands[operand_index].value.unsigned_integer = current_token.value.unsigned_integer;

					stream::peek(stream); // Numeric value
				}
				else if (current_token.type == Token_Type::NUMERIC_LITERAL_UI16) {
					operands[operand_index].type_flags = Operand::Type_Flags::IMMEDIATE;
					operands[operand_index].size = Operand::Size::WORD;

					operands[operand_index].value.unsigned_integer = current_token.value.unsigned_integer;

					stream::peek(stream); // Numeric value
				}
				else if (current_token.type == Token_Type::NUMERIC_LITERAL_UI32) {
					operands[operand_index].type_flags = Operand::Type_Flags::IMMEDIATE;
					operands[operand_index].size = Operand::Size::DOUBLE_WORD;

					operands[operand_index].value.unsigned_integer = current_token.value.unsigned_integer;

					stream::peek(stream); // Numeric value
				}
				else if (current_token.type == Token_Type::NUMERIC_LITERAL_UI64) {
					operands[operand_index].type_flags = Operand::Type_Flags::IMMEDIATE;
					operands[operand_index].size = Operand::Size::QUAD_WORD;

					operands[operand_index].value.unsigned_integer = current_token.value.unsigned_integer;

					stream::peek(stream); // Numeric value
				}
				else if (current_token.type == Token_Type::NUMERIC_LITERAL_F32) {
					operands[operand_index].type_flags = Operand::Type_Flags::IMMEDIATE;
					operands[operand_index].size = Operand::Size::DOUBLE_WORD;

					operands[operand_index].value.real_32 = current_token.value.real_32;

					stream::peek(stream); // Numeric value
				}
				else if (current_token.type == Token_Type::NUMERIC_LITERAL_F64) {
					operands[operand_index].type_flags = Operand::Type_Flags::IMMEDIATE;
					operands[operand_index].size = Operand::Size::QUAD_WORD;

					operands[operand_index].value.real_64 = current_token.value.real_64;

					stream::peek(stream); // Numeric value
				}
				else if (current_token.type == Token_Type::NUMERIC_LITERAL_REAL) {
					operands[operand_index].value.real_max = current_token.value.real_max;

					stream::peek(stream); // Numeric value
				}
				else {
					report_error(Compiler_Error::error, current_token, "Syntaxe error, unrecognized token type during instruction parsing.");
				}
			}

			current_token = stream::get(stream);
		}

		if (!push_instruction(section, instruction, operands)) {
			report_error(Compiler_Error::error, instruction_token, "Unable to find encoding description for the instruction .. with operands .. and ...");
		}

		if (stream::is_eof(stream)) {
			// @TODO Test and improve this error message
			report_error(Compiler_Error::error, current_token, "End of file reached before the closure of the section");
		}
	}

	void parse_label(ASM& asm_result, stream::Array_Read_Stream<Token>& stream, Section* section)
	{
		ZoneScopedNC("f::ASM::parse_label", 0x1b5e20);

		Token	current_token;
		Token	label_name;

		label_name = stream::get<Token>(stream);
		stream::peek<Token>(stream);	// label name

		Label** found_label;

		uint64_t label_hash = SpookyHash::Hash64((const void*)fstd::language::to_utf8(label_name.text), fstd::language::get_string_size(label_name.text), 0);
		uint16_t label_short_hash = label_hash & 0xffff;

		found_label = fstd::memory::hash_table_get(asm_result.labels, label_short_hash, label_name.text);
		if (found_label) {
			// @TODO give the position of the first definition
			report_error(Compiler_Error::error, label_name, "Label already used.");
		}

		Label* new_label = allocate_Label();

		new_label->label = label_name.text;
		new_label->section = section;
		new_label->RVA = stream::get_position(section->stream_data);

		found_label = fstd::memory::hash_table_insert(asm_result.labels, label_short_hash, label_name.text, new_label);

		current_token = stream::get<Token>(stream);
		if (!(current_token.type == Token_Type::SYNTAXE_OPERATOR
			&& current_token.value.punctuation == Punctuation::COLON)) {
			report_error(Compiler_Error::error, label_name, "Syntax error, colon is expected after the label identifier.");
		}
		stream::peek<Token>(stream);	// :
	}

	void parse_section(ASM& asm_result, stream::Array_Read_Stream<Token>& stream)
	{
		ZoneScopedNC("f::ASM::parse_section", 0x1b5e20);

		Token		current_token;
		Token		section_name;
		Section*	section;

		stream::peek<Token>(stream);	// SECTION

		section_name = stream::get(stream);
		if (section_name.type != Token_Type::STRING_LITERAL) {
			report_error(Compiler_Error::error, section_name, "Missing string after \"SECTION\" keyword, you should specify the name of the section.");
		}
		if (get_string_size(section_name.text) > SECTION_NAME_MAX_LENGTH) {
			report_error(Compiler_Error::error, section_name, "Section name can't be larger than 8 characters (PE file format is the most restrecting supported format here).");
		}
		section = create_section(asm_result, section_name.text);

		stream::peek<Token>(stream);	// section name

		current_token = stream::get(stream);
		if (current_token.type != Token_Type::SYNTAXE_OPERATOR
			&& current_token.value.punctuation != Punctuation::OPEN_BRACE) {
			report_error(Compiler_Error::error, current_token, "Missing '{' character after \"SECTION\" keyword!");
		}

		stream::peek<Token>(stream);	// {

		while (stream::is_eof(stream) == false)
		{
			current_token = stream::get(stream);

			if (current_token.type == Token_Type::SYNTAXE_OPERATOR
				&& current_token.value.punctuation == Punctuation::CLOSE_BRACE) {
				stream::peek<Token>(stream);	// }
				return;
			}
			else if (current_token.type == Token_Type::KEYWORD) {
				parse_pseudo_instruction(asm_result, stream, section);	// @warning should be a pseudo instruction
			}
			else if (current_token.type == Token_Type::INSTRUCTION) {
				parse_instruction(asm_result, stream, section);
			}
			else if (current_token.type == Token_Type::IDENTIFIER) {
				parse_label(asm_result, stream, section);
			}
			else {
				report_error(Compiler_Error::error, current_token, "A label or an instruction (or pseudo instruction) is expected"); // @TODO improve this error message (should tell more about what it is supported in this context)
			}
		}

		report_error(Compiler_Error::error, current_token, "End of file reached. Missing '}' to delimite the \"SECTION\" scope.");
	}

	void compile(ASM& asm_result, stream::Array_Read_Stream<Token>& stream)
	{
		ZoneScopedNC("f::ASM::compile", 0x1b5e20);

		Token	current_token;

		while (stream::is_eof(stream) == false)
		{
			current_token = stream::get(stream);

			if (current_token.type == Token_Type::KEYWORD) {
				if (current_token.value.keyword == Keyword::IMPORTS) {
					parse_import(asm_result, stream);
				}
				else if (current_token.value.keyword == Keyword::SECTION) {
					parse_section(asm_result, stream);
				}
				else {
					report_error(Compiler_Error::error, current_token, "Unsupported keyword a this scope level.");
				}
			}
			else {
				report_error(Compiler_Error::error, current_token, "A keyword is expected, you should start a SECTION or an IMPORTS scope.");
			}
		}
	}

	Section* create_section(ASM& asm_result, fstd::language::string_view name)
	{
		ZoneScopedNC("f::ASM::create_section", 0x1b5e20);

		core::Assert(get_string_size(name) <= SECTION_NAME_MAX_LENGTH);

		// Return existing section
		size_t search_result = memory::find_array_element(asm_result.sections, name, &match_section);
		if (search_result != (size_t)-1) {
			return memory::get_array_element(asm_result.sections, search_result);
		}

		memory::array_push_back(asm_result.sections, Section());
		Section* new_section = memory::get_array_last_element(asm_result.sections);

		new_section->name = name;
		stream::init(new_section->stream_data);
		return new_section;
	}

	inline size_t get_first_instruction_desc_index(Instruction instruction)
	{
		return g_instruction_desc_table_indices[(size_t)instruction];
	}

	inline size_t get_next_instruction_first_desc(Instruction instruction)
	{
		return g_instruction_desc_table_indices[(size_t)instruction + 1];
	}

	inline void encode_additionnal_bit_in_REX_prefix(uint8_t value, uint8_t data[64], uint8_t REX_prefix_index, uint8_t target_bit)
	{
		if (value > 0b111) {
			if (REX_prefix_index == (uint8_t)-1) {
				report_error(Compiler_Error::internal_error, "??? Is it really valid to use a 64bit register without the REX prefix. If not turn this into a real error!");
			}

			// If it crash here it is because there is no REX prefix and REX_prefix_index == -1
			// I am not sure it can happens with valid code (using a 64bit register without the prefix to set the 64bit addressing mode)
			data[REX_prefix_index] |= target_bit;
		}
	}

	void encode_operand(uint8_t operand_encoding, uint8_t REX_prefix_index, uint8_t last_opcode_byte_index, uint8_t modr_value, uint8_t data[64], uint8_t& size, const Operand_Encoding_Desc& desc_operand, const Operand& operand)
	{
		// @TODO faire des fonctions pour encoder les modrm ça sera plus clair
		// avec mod, reg et rm

		// modr / m struct :
		//     (most significant bit)
		//     2 bits       mod - 00 = > indirect, e.g.[eax]
		//     01 = > indirect plus byte offset
		//     10 = > indirect plus word offset
		//     11 = > register
		//     3 bits       reg - identifies register
		//     3 bits       rm - identifies second register or additional data (often addressing mode)
		//     (least significant bit)

		uint8_t* modrm = nullptr;
		if (is_flag_set(desc_operand.encoding_flags, (uint8_t)Operand_Encoding_Desc::Encoding_Flags::REGISTER_MODR)) {
			// "Allocate" the modrm byte just behind the last opcode byte ("fixed" position) if necessary
			// modrm can contains 2 register operands
			modrm = &data[last_opcode_byte_index + 1];
			if (last_opcode_byte_index + 1 >= size) {
				*modrm = 0;
				size++;	// This is what does the "allocation"
			}
		}

		// First operand encoded in modr/m struct
		if (operand.type_flags == (uint8_t)Operand::Type_Flags::REGISTER) {
			if (is_flag_set(desc_operand.encoding_flags, (uint8_t)Operand_Encoding_Desc::Encoding_Flags::REGISTER_MODR)) {
				if (modr_value == (uint8_t)-2) {
					if (is_flag_set(operand_encoding, (uint8_t)Instruction_Desc::Operand_Encoding_Flags::IN_MODRM_REG)) {
						*modrm |= 0b11 << 6; // register flag
						*modrm |= (g_register_desc_table[(size_t)operand.value._register].id & 0b111) << 3;

						encode_additionnal_bit_in_REX_prefix(g_register_desc_table[(size_t)operand.value._register].id, data, REX_prefix_index, 0b0100);
					}
					else if (is_flag_set(operand_encoding, (uint8_t)Instruction_Desc::Operand_Encoding_Flags::IN_MODRM_RM)) {
						*modrm |= (g_register_desc_table[(size_t)operand.value._register].id & 0b111);

						encode_additionnal_bit_in_REX_prefix(g_register_desc_table[(size_t)operand.value._register].id, data, REX_prefix_index, 0b0001);
					}
					else if (is_flag_set(operand_encoding, (uint8_t)Instruction_Desc::Operand_Encoding_Flags::IMPLICIT_OPERAND)) {
					}
					else {
						report_error(Compiler_Error::internal_error, "Register encoding fails for a strange reason or unsupported encoding rule");
					}
				}
				else {
					*modrm |= 0b11 << 6; // register flag
					*modrm |= (modr_value & 0b111) << 3;
					*modrm |= (g_register_desc_table[(size_t)operand.value._register].id & 0b111);

					encode_additionnal_bit_in_REX_prefix(g_register_desc_table[(size_t)operand.value._register].id, data, REX_prefix_index, 0b0001);
				}
			}
			else if (is_flag_set(desc_operand.encoding_flags, (uint8_t)Operand_Encoding_Desc::Encoding_Flags::REGISTER_ADD_TO_OPCODE)) {
				if (g_register_desc_table[(size_t)operand.value._register].id > 0b111) {
					// @TODO should be an assert I think
//					report_error(Compiler_Error::internal_error, "encoding the register directly in the opcode seems to be only a thing with a 32bit register!");
					// We certainly pick the wrong instruction description here, fix the matching method
				}

				data[last_opcode_byte_index] += g_register_desc_table[(size_t)operand.value._register].id;
			}
			else {
				report_error(Compiler_Error::internal_error, "Register encoding fails for a strange reason or unsupported encoding rule");
			}
		}
		else if (operand.type_flags == (uint8_t)Operand::Type_Flags::IMMEDIATE) {
			// desc_operand give us the number of bytes to write (in some cases we use an instruction supporting a bigger immediate that needed
			// because it may have not a version with the size we want)
			if (desc_operand.op.size == Operand::Size::BYTE) {
				data[size++] = (uint8_t)operand.value.integer;
			}
			else if (desc_operand.op.size == Operand::Size::WORD) {
				uint16_t value = (uint16_t)operand.value.integer;
				for (uint8_t i = 0; i < 2; i++) {
					data[size++] = ((uint8_t*)&value)[i];
				}
			}
			else if (desc_operand.op.size == Operand::Size::DOUBLE_WORD) {
				uint32_t value = (uint32_t)operand.value.integer;
				for (uint8_t i = 0; i < 4; i++) {
					data[size++] = ((uint8_t*)&value)[i];
				}
			}
			else if (desc_operand.op.size == Operand::Size::QUAD_WORD
				|| desc_operand.op.size == Operand::Size::ADDRESS_SIZE) { // @Warning x64 as target
				uint64_t value = operand.value.integer;
				for (uint8_t i = 0; i < 8; i++) {
					data[size++] = ((uint8_t*)&value)[i];
				}
			}
			else {
				// @TODO improve this error message (handle flags)
				// We may have to return false and print the error in the caller to have access to tokens
				report_error(Compiler_Error::error, "Operand size not supported");
			}
		}
		else if (operand.type_flags == (uint8_t)Operand::Type_Flags::ADDRESS) {
			// Check at https://stackoverflow.com/questions/15511482/x64-instruction-encoding-and-the-modrm-byte
			// for the mod part of modrm byte
			if (is_flag_set(desc_operand.encoding_flags, (uint8_t)Operand_Encoding_Desc::Encoding_Flags::REGISTER_MODR)) {
				*modrm &= 0b00111111;	// Remove mod flag (that can contains 0b11 from a previous register operand)

				// Some instruction like CALL ("[202] CALL mem [m: odf ff /2] 8086,BND") need a modrm value
				if (modr_value != (uint8_t)-1
					&& modr_value != (uint8_t)-2) {
					*modrm |= (modr_value & 0b111) << 3;
				}
				// More info about the displacement in 64bits mode:
				// https://xem.github.io/minix86/manual/intel-x86-and-64-manual-vol2/o_b5573232dd8f1481-72.html
				*modrm |= 0b101;	// Displacement mode (make the address relative to register RIP (Re-Extended Instruction Pointer))
			}

			// @TODO @Warning be clear about what we are doing here
			// often the address is in 32bits but added to a register, is it what means "rm*" operand type? m for memory
			// displacement to a register value? with RIP (Instruction Pointer) we can get a 64bits address
			uint32_t value = (uint32_t)operand.value.integer;
//			uint32_t value = 0xDEADBEEF;
			for (uint8_t i = 0; i < 4; i++) {
				data[size++] = ((uint8_t*)&value)[i];
			}
		}
	}

	inline bool is_the_instruction_desc_compatible(const Instruction_Desc& instruction_desc, const Operand operands[2])
	{
		// @TODO handle flags,...
		// immediate size value promotions

		for (uint8_t i = 0; i < NB_MAX_OPERAND_PER_INSTRUCTION; i++)
		{
			if (((is_flag_set(instruction_desc.op_enc_descs[i].op.type_flags, operands[i].type_flags) && instruction_desc.op_enc_descs[i].op.size >= operands[i].size)
				|| (operands[i].type_flags == NONE && operands[i].size == Operand::Size::NONE
					&& instruction_desc.op_enc_descs[i].op.type_flags == NONE && instruction_desc.op_enc_descs[i].op.size == Operand::Size::NONE))) {
			}
			else {
				return false;
			}
		}
		return true;
	}

	bool push_instruction(Section* section, Instruction instruction, const Operand operands[NB_MAX_OPERAND_PER_INSTRUCTION])
	{
		// Find the instruction decription that match parameters
		size_t desc_index = get_first_instruction_desc_index(instruction);
		for (; desc_index < get_next_instruction_first_desc(instruction); desc_index++)
		{
			if (is_the_instruction_desc_compatible(g_instruction_desc_table[desc_index], operands)) {
				break;
			}
		}
		if (desc_index == get_next_instruction_first_desc(instruction)) {
			// @TODO improve this error message (handle flags)
			// We may have to return false and print the error in the caller to have access to tokens
			return false;
		}

		Instruction_Desc	instruction_desc = g_instruction_desc_table[desc_index];
		uint8_t				data[64];
		uint8_t				REX_prefix_index = -1;
		uint8_t				size = 0;

		// REX prefix (only the W member can be set at instruction level, other members are set at operands level)
		if (is_flag_set(instruction_desc.encoding_flags, (uint8_t)Instruction_Desc::Encoding_Flags::PREFIX_REX_W)) {
			// W member should be 1 to switch to 64bit addressing mode
			data[size++] = 0b0100 << 4 | 0b1000; // first 4bits are the prefix bit pattern
			REX_prefix_index = 0;
		}

		for (uint8_t i = 0; i < instruction_desc.opcode_size; i++) {
			data[size++] = ((uint8_t*)&instruction_desc.opcode)[i];
		}

		uint8_t last_opcode_byte_index = size - 1;
		for (uint8_t op_i = 0; op_i < NB_MAX_OPERAND_PER_INSTRUCTION; op_i++)
		{
			encode_operand(
				instruction_desc.operands_encoding[op_i],
				REX_prefix_index,
				last_opcode_byte_index,
				instruction_desc.modr_value,
				data,
				size,
				instruction_desc.op_enc_descs[op_i],
				operands[op_i]);
		}

		push_raw_data(section, data, size);

		// @TODO continuer de definir la struct Operand
		// Voir comment supporter l'encodage de l'instruction lea, je ne comprend pas l'operation sur la 2eme operande qui est une immediate value

		// @TODO handle label and addresses (by adding a new ADDR_TO_PATCH)
		// Function names have certainly to be handled like labels and trigger conflicts with labels
		return true;
	}

	void push_raw_data(Section* section, uint8_t* data, uint32_t size)
	{
		ZoneScopedNC("f::ASM::push_raw_data", 0x1b5e20);

		stream::write(section->stream_data, data, size);
	}

	Section* get_section(const ASM& asm_result, fstd::language::string_view name)
	{
		size_t search_result = memory::find_array_element(asm_result.sections, name, &match_section);
		if (search_result != (size_t)-1) {
			return memory::get_array_element(asm_result.sections, search_result);
		}
		return nullptr;
	}

	void debug_print(const ASM& asm_result)
	{
		ZoneScopedNC("f::ASM::debug_print", 0x1b5e20);

		// @TODO output all the data for debugging purpose
		// The printed ASM should match what a debugger with a disasembler output
	}
}
