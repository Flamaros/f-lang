#include "ASM.hpp"

namespace f::ASM
{
	void compile_file(const fstd::system::Path& path, const fstd::system::Path& output_path, bool shared_library)
	{
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
	}
}
