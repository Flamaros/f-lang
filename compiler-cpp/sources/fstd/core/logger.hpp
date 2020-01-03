#pragma once

#include <cstdint>

namespace fstd
{
	namespace core
	{
		enum class Log_Level {
			verbose,
			info,
			warning,
			error,
			invalid
		};

		struct Logger {
			Log_Level   level = Log_Level::warning;
			uint8_t		indentation = 0;
		};

		void        set_log_level(Logger& logger, Log_Level level);
		Log_Level   get_log_level(Logger& logger);
		void        log(Logger& logger, Log_Level level, const char* format, ...);
		void		log_push_scope(Logger& logger, const char* scope_name);
		void		log_pop_scope(Logger& logger);
		void        flush_logs(Logger& logger);
	}
}
