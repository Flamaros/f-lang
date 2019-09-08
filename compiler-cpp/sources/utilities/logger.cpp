#include "logger.hpp"

#include <platform.hpp>

#include <cstdio>

#include <stdarg.h>

#if defined(FL_WIN32) && defined(FL_DEVELOPER_MODE)
#   include <Windows.h>

#   include <fcntl.h>
#   include <io.h>
#   include <iostream>
#   include <fstream>
#endif

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

void flush_logs(Logger* logger) {
    if (logger == nullptr)
        return;
    fflush(stdout);
}

void open_log_console() {
#if defined(FL_WIN32) && defined(FL_DEVELOPER_MODE)
    //Create a console for this application
    AllocConsole();

    // Get STDOUT handle
    HANDLE ConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    int SystemOutput = _open_osfhandle(intptr_t(ConsoleOutput), _O_TEXT);
    FILE *COutputHandle = _fdopen(SystemOutput, "w");

    // Get STDERR handle
    HANDLE ConsoleError = GetStdHandle(STD_ERROR_HANDLE);
    int SystemError = _open_osfhandle(intptr_t(ConsoleError), _O_TEXT);
    FILE *CErrorHandle = _fdopen(SystemError, "w");

    // Get STDIN handle
    HANDLE ConsoleInput = GetStdHandle(STD_INPUT_HANDLE);
    int SystemInput = _open_osfhandle(intptr_t(ConsoleInput), _O_TEXT);
    FILE *CInputHandle = _fdopen(SystemInput, "r");

    //make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog point to console as well
    std::ios::sync_with_stdio(true);

    // Redirect the CRT standard input, output, and error handles to the console
    freopen_s(&CInputHandle, "CONIN$", "r", stdin);
    freopen_s(&COutputHandle, "CONOUT$", "w", stdout);
    freopen_s(&CErrorHandle, "CONOUT$", "w", stderr);

    //Clear the error state for each of the C++ standard stream objects. We need to do this, as
    //attempts to access the standard streams before they refer to a valid target will cause the
    //iostream objects to enter an error state. In versions of Visual Studio after 2005, this seems
    //to always occur during startup regardless of whether anything has been read from or written to
    //the console or not.
    std::wcout.clear();
    std::cout.clear();
    std::wcerr.clear();
    std::cerr.clear();
    std::wcin.clear();
    std::cin.clear();
#endif
}
