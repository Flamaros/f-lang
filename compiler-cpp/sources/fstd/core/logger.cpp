#include "logger.hpp"

#include <cstdio>

#include <stdarg.h>

namespace fstd
{
	namespace core
	{
		void set_log_level(Logger* logger, Log_Level level) {
			if (logger == nullptr)
				return;

			logger->level = level;
		}

		Log_Level get_log_level(Logger* logger) {
			if (logger == nullptr)
				return Log_Level::invalid;

			return logger->level;
		}

		void log(Logger* logger, Log_Level level, const char* format, ...) {
			if (logger == nullptr
				|| level < logger->level)
				return;

			va_list args;

			va_start(args, format);

			// @TODO Mettre en place un mécanisme de thread safety
			// Il faut flush pour débloquer le thread courant
			// Encoder directement les couleurs dans la string avec des codes VT_100
			// bufferiser dans un buffer par thread ou locker pour le premier thread qui appelle log apres un flush

			for (uint8_t i = 0; i < logger->indentation; i++) {
				printf("    ");
			}

			switch (level) {
			case Log_Level::verbose:
				printf("\033[38;5;14mVerbose:\033[0m ");
				break;
			case Log_Level::info:
				printf("\033[38;5;10mInfo:\033[0m ");
				break;
			case Log_Level::warning:
				printf("\033[38;5;208mWarning:\033[0m ");
				break;
			case Log_Level::error:
				printf("\033[38;5;196;4mError:\033[0m ");
				break;
			default:
				break;
			}
			vprintf(format, args);

			va_end(args);
		}

		void log_push_scope(Logger* logger, const char* scope_name) {
			if (logger == nullptr)
				return;

			for (uint8_t i = 0; i < logger->indentation; i++) {
				printf("    ");
			}
			printf("%s:\n", scope_name);

			logger->indentation++;
		}

		void log_pop_scope(Logger* logger) {
			if (logger == nullptr)
				return;

			logger->indentation--;
		}

		void flush_logs(Logger* logger) {
			if (logger == nullptr)
				return;
			fflush(stdout);
		}
	}
}