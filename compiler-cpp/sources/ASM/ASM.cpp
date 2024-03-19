#include "ASM.hpp"

#include "ASM_lexer.hpp"
#include "globals.hpp"

#include <fstd/language/defer.hpp>
#include <fstd/memory/array.hpp>
#include <fstd/system/file.hpp>
#include <fstd/stream/array_read_stream.hpp>

#include <third-party/SpookyV2.h>

using namespace fstd;

#define SECTION_NAME_MAX_LENGTH	8	// 8 is the maximum supported by PE file format

#define NB_PREALLOCATED_IMPORTED_LIBRARIES				128
#define NB_PREALLOCATED_IMPORTED_FUNCTIONS_PER_LIBRARY	4096
#define NB_PREALLOCALED_SECTIONS						5		// .text, .rdata, .data, .reloc, .idata

namespace f::ASM
{
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

			memory::init(asm_result.sections);
			memory::reserve_array(asm_result.sections, NB_PREALLOCALED_SECTIONS);
		}

		// Working data (stored in globals)
		{
			memory::init(globals.asm_data.imported_libraries);
			memory::reserve_array(globals.asm_data.imported_libraries, NB_PREALLOCATED_IMPORTED_LIBRARIES);

			memory::init(globals.asm_data.imported_functions);
			memory::reserve_array(globals.asm_data.imported_functions, NB_PREALLOCATED_IMPORTED_LIBRARIES * NB_PREALLOCATED_IMPORTED_FUNCTIONS_PER_LIBRARY);
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
		// Je n'ai pas besoin de générer un AST
		//   - Il faut simplement remplir des structures qui correspondent aux différentes parties du binaire
		//     par exemple il faut pouvoir fragmenter la déclaration des imports de modules, pareil pour les sections
		//   - Il faut sans doute une table de label par section
		//     un label n'est qu'une key dans une table de hash indiquant l'offset par rapport au début de la section
		//	   afin de pouvoir utiliser un label avant sa déclaration il faut avoir une liste des addresse à patcher
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
		//		Un label est équivalent à une addresse, si j'ai un symbole comme $ pour récupérer l'addresse de l'instruction
		//		actuelle, je dois pouvoir implémenter des opérations simple sans AST et sans gestion de précédence sur les opérateurs.
		//		Car le but est de pouvoir éventuellement faire:
		//			message:	db "Hello World"		// message est un label qui a une addresse
		//			len:		dd	$ - message			// $ est l'addresse de l'instruction courante
		//		Dans l'exemple de code précédent je n'ai absolument pas besoin de me soucier de la précédence des opérateurs
		//		Car l'opération est trop simple, je ne suis pas sur d'avoir besoin de plus pour la section .data
		//		Et encore comme mon ASM vise à être généré, normalement le front-end de mon language n'aura même pas
		//		besoin de cette fonctionnalité, c'est pour ça que c'est du pure bonus pour mes dev.
	}

	void parse_module(ASM& asm_result, stream::Array_Read_Stream<Token>& stream)
	{
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
		size_t	starting_line;
		Token	current_token;

		current_token = stream::get(stream);
		starting_line = current_token.line;
		fstd::core::Assert(
			current_token.type == Token_Type::KEYWORD
			&& current_token.value.keyword >= Keyword::DB
			&& current_token.value.keyword < Keyword::COUNT);
		while (stream::is_eof(stream) == false)
		{
			// @TODO handle the instruction parsing
			if (current_token.value.keyword == Keyword::DB) { // @TODO put it in a specific function?
				stream::peek(stream);	// db
				current_token = stream::get(stream);
				// @TODO loop to handle , operand separator
				if (current_token.type == Token_Type::STRING_LITERAL
					|| current_token.type == Token_Type::STRING_LITERAL_RAW) {
					push_raw_data(section, to_utf8(*current_token.value.string), (uint32_t)get_string_size(*current_token.value.string));
				}
				else if (current_token.type == Token_Type::NUMERIC_LITERAL_I32) {
					if (current_token.value.integer < 0 || current_token.value.integer > 256) {
						report_error(Compiler_Error::error, current_token, "Operand value out of range: if you specify a character by its value it should be in range [0-256].");
					}
					unsigned char value = (unsigned char)current_token.value.integer;
					push_raw_data(section, &value, sizeof(value));
				}
			}
			else {
				stream::peek(stream);
				current_token = stream::get(stream);
			}

			if (current_token.line > starting_line) {
				break;
			}
		}
	}

	void parse_instruction(ASM& asm_result, stream::Array_Read_Stream<Token>& stream, Section* section)
	{
		size_t	starting_line;
		Token	current_token;

		current_token = stream::get(stream);
		starting_line = current_token.line;
		while (stream::is_eof(stream) == false)
		{
			// @TODO handle the instruction parsing

			stream::peek(stream);
			current_token = stream::get(stream);
			if (current_token.line > starting_line) {
				break;
			}
		}
	}

	void parse_label(ASM& asm_result, stream::Array_Read_Stream<Token>& stream)
	{
		stream::peek<Token>(stream);	// label name

		// @TODO check that it is the right token here, else throw a parsing error
		stream::peek<Token>(stream);	// :
	}

	void parse_section(ASM& asm_result, stream::Array_Read_Stream<Token>& stream)
	{
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
				parse_label(asm_result, stream);
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

	void push_instruction(Section* section, uint16_t instruction, const Operand& operand1, const Operand& operand2)
	{
		// @TODO continuer de definir la struct Operand
		// Voir comment supporter l'encodage de l'instruction lea, je ne comprend pas l'operation sur la 2eme operande qui est une immediate value
	}

	void push_raw_data(Section* section, uint8_t* data, uint32_t size)
	{
		stream::write(section->stream_data, data, size);
	}

	void debug_print(const ASM& asm_result)
	{
		// @TODO output all the data for debugging purpose
		// The printed ASM should match what a debugger with a disasembler output
	}
}
