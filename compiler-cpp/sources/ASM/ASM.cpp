#include "ASM.hpp"

#include "ASM_lexer.hpp"
#include "globals.hpp"

#include <fstd/language/defer.hpp>
#include <fstd/memory/array.hpp>
#include <fstd/system/file.hpp>

using namespace fstd;

namespace f::ASM
{
	void compile_file(const fstd::system::Path& path, const fstd::system::Path& output_path, bool shared_library)
	{
		ZoneScopedNC("f::lex", 0x1b5e20);

		fstd::memory::Array<Token>	tokens;

		initialize_lexer();
		lex(path, tokens);
		print(tokens);



		// @TODO
		// Me faire un lexer
		//   - Dissocier les keywords et les instructions?
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
}
