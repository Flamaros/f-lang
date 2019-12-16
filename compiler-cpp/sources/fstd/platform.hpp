#pragma once

#if defined(_WIN32)
#	define OS_WINDOWS
#elif defined(__unix__) || defined(__linux) || defined(__linux__) || defined(__ANDROID__) || defined(__QNX__)
#	define OS_POSIX_COMPATIBLE
#	if defined(__unix__)
#		define OS_UNIX
#	elif defined(__linux) || defined(__linux__)
#		define OS_LINUX
#	elif defined(__ANDROID__)
#		define OS_ANDROID
#	elif defined(__QNX__)
#		define OS_QNX
#	endif
#elif defined(__EPOC32__)
#	define OS_SYMBIAN	// @TODO @Warning check if this is really the symbian flag
#elif defined(__APPLE__)
#	define OS_MACOS
#else
#	error "Unreconized OS"
#endif
