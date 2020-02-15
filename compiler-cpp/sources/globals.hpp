#pragma once

namespace fstd
{
	namespace core
	{
		struct Logger;
	}
}

struct Globals
{
	fstd::core::Logger*	logger = nullptr;
};

void initialize_globals();

extern thread_local Globals    globals;
