#include "globals.hpp"

#include <fstd/core/logger.hpp>

thread_local Globals    globals;

void initialize_globals()
{
	globals.logger = new fstd::core::Logger();
}
