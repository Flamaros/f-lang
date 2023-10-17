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
}
