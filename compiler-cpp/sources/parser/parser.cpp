#include "parser.hpp"

#include "globals.hpp"

#include <fstd/memory/array.hpp>

#include <fstd/stream/array_stream.hpp>

#undef max
#include <tracy/Tracy.hpp>

// Typedef complexity
// https://en.cppreference.com/w/cpp/language/typedef

using namespace fstd;

namespace f
{
	enum class State
	{
		GLOBAL_SCOPE,
		COMMENT_LINE,
		COMMENT_BLOCK,
        IMPORT_DIRECTIVE,

		eof
	};

	void parse(fstd::memory::Array<Token>& tokens, AST& ast)
	{
		ZoneScopedNC("f::parse", 0xff6f00);

		stream::Array_Stream<Token>	stream;

		// It is impossible to have more nodes than tokens, so we can easily pre-allocate them.
		//
		// Flamaros - 23 march 2020
		memory::reserve_array(globals.ast_nodes, memory::get_array_size(tokens));

		stream::initialize_memory_stream<Token>(stream, tokens);

		if (stream::is_eof(stream) == true) {
			return;
		}

		while (stream::is_eof(stream) == false)
		{
			stream::peek<Token>(stream);
		}
	}
}
