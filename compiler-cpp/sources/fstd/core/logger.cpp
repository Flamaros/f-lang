#include "logger.hpp"

#include <fstd/language/string.hpp>
#include <fstd/language/string_view.hpp>
#include <fstd/language/defer.hpp>

#include <fstd/system/stdio.hpp>

#include <fstd/core/string_builder.hpp>

#include <cstdarg>	// @TODO remove it

using namespace fstd::language;
using namespace fstd::system;

namespace fstd
{
	namespace core
	{
		void set_log_level(Logger& logger, Log_Level level) {
			logger.level = level;
		}

		Log_Level get_log_level(Logger& logger) {
			return logger.level;
		}

		void log(Logger& logger, Log_Level level, const char* format, ...) {
			if (level < logger.level)
				return;

			va_list args;

			core::String_Builder	string_builder;
			language::string		format_string;
			language::string		formatted_string;

			va_start(args, format);

			defer{
				core::free_buffers(string_builder);
				language::release(formatted_string);
			};

			// @TODO
			// Bufferize the String_Builder to delay the print to the flush.
			// By storing String_Builder per thread we can have a thread-safe logger.
			//
			// Flamaros - 19 february 2020
			language::string_view   header;

			language::assign(header, (uint8_t*)"    ");
			for (uint8_t i = 0; i < logger.indentation; i++) {
				system::print(header);
			}

			switch (level) {
			case Log_Level::verbose:
				language::assign(header, (uint8_t *)"\033[38;5;14mVerbose:\033[0m ");
				break;
			case Log_Level::info:
				language::assign(header, (uint8_t *)"\033[38;5;10mInfo:\033[0m ");
				break;
			case Log_Level::warning:
				language::assign(header, (uint8_t *)"\033[38;5;208mWarning:\033[0m ");
				break;
			case Log_Level::error:
				language::assign(header, (uint8_t *)"\033[38;5;196;4mError:\033[0m ");
				break;
			default:
				fstd::core::Assert(false);
				break;
			}
			system::print(header);

			language::assign(format_string, (uint8_t*)format);
			core::print_to_builder(string_builder, &format_string, args);

			formatted_string = core::to_string(string_builder);
			system::print(formatted_string);

			va_end(args);
		}

		void log_push_scope(Logger& logger, const char* scope_name) {
			language::string_view   indentation;

			language::assign(indentation, (uint8_t*)"    ");
			for (uint8_t i = 0; i < logger.indentation; i++) {
				system::print(indentation);
			}
			system::print(indentation);

			// Print the name of the scope
			core::String_Builder	string_builder;
			language::string		format;
			language::string		formatted_string;

			defer{
				core::free_buffers(string_builder);
				language::release(formatted_string);
			};

			language::assign(format, (uint8_t*)"%s:\n");
			core::print_to_builder(string_builder, &format, scope_name);

			formatted_string = core::to_string(string_builder);
			system::print(formatted_string);

			logger.indentation++;
		}

		void log_pop_scope(Logger& logger) {
			logger.indentation--;
		}

		void flush_logs(Logger& logger) {
			// @TODO add the flug in system::stdio module
			//fflush(stdout);
		}
	}
}
