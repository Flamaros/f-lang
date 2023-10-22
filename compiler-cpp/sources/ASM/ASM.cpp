#include "ASM.hpp"

#include "ASM_lexer.hpp"
#include "globals.hpp"

#include <fstd/language/defer.hpp>
#include <fstd/memory/array.hpp>
#include <fstd/system/file.hpp>

using namespace fstd;

namespace f::ASM
{
	void compile(stream::Array_Stream<Token>& stream);

	void compile_file(const fstd::system::Path& path, const fstd::system::Path& output_path, bool shared_library)
	{
		ZoneScopedNC("f::ASM::compile_file", 0x1b5e20);

		fstd::memory::Array<Token>	tokens;

		initialize_lexer();
		lex(path, tokens);

#if defined(FSTD_DEBUG)
		print(tokens);
#endif

		stream::Array_Stream<Token>	stream;

		stream::initialize_memory_stream<Token>(stream, tokens);

		compile(stream);

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

	void parse_module(stream::Array_Stream<Token>& stream)
	{
		Token	current_token;
		Token	module_name;

		stream::peek<Token>(stream);	// MODULE

		module_name = stream::get(stream);
		if (module_name.type != Token_Type::STRING_LITERAL) {
			report_error(Compiler_Error::error, module_name, "Missing string after \"MODULE\" keyword, you should specify the name of the module.");
		}

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

			// @TODO remove that
			stream::peek<Token>(stream);
		}

		report_error(Compiler_Error::error, current_token, "End of file reached. Missing '}' to delimite the \"MODULE\" scope.");
	}

	void parse_import(stream::Array_Stream<Token>& stream)
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
					parse_module(stream);
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

	void parse_section(stream::Array_Stream<Token>& stream)
	{
		Token	current_token;
		Token	section_name;

		stream::peek<Token>(stream);	// SECTION

		section_name = stream::get(stream);
		if (section_name.type != Token_Type::STRING_LITERAL) {
			report_error(Compiler_Error::error, section_name, "Missing string after \"SECTION\" keyword, you should specify the name of the section.");
		}

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

			// @TODO remove that
			stream::peek<Token>(stream);
		}

		report_error(Compiler_Error::error, current_token, "End of file reached. Missing '}' to delimite the \"SECTION\" scope.");
	}

	void compile(stream::Array_Stream<Token>& stream)
	{
		ZoneScopedNC("f::ASM::compile", 0x1b5e20);

		Token	current_token;

		while (stream::is_eof(stream) == false)
		{
			current_token = stream::get(stream);

			if (current_token.type == Token_Type::KEYWORD) {
				if (current_token.value.keyword == Keyword::IMPORTS) {
					parse_import(stream);
				}
				else if (current_token.value.keyword == Keyword::SECTION) {
					parse_section(stream);
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
}
