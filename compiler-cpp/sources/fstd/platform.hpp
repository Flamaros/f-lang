#pragma once

// OS
#if defined(_WIN32)
#	define FSTD_OS_WINDOWS
#elif defined(__unix__) || defined(__linux) || defined(__linux__) || defined(__ANDROID__) || defined(__QNX__)
#	define FSTD_OS_POSIX_COMPATIBLE
#	if defined(__unix__)
#		define FSTD_OS_UNIX
#	elif defined(__linux) || defined(__linux__)
#		define FSTD_OS_LINUX
#	elif defined(__ANDROID__)
#		define FSTD_OS_ANDROID
#	elif defined(__QNX__)
#		define FSTD_OS_QNX
#	endif
#elif defined(__EPOC32__)
#	define FSTD_OS_SYMBIAN	// @TODO @Warning check if this is really the symbian flag
#elif defined(__APPLE__)
#	define FSTD_OS_MACOS
#else
#	error "Unreconized OS"
#endif

// Architectures
#if defined(__arm__)
#   define FSTD_ARM
#elif defined(__i386__) || defined(_M_IX86)
#   define FSTD_X86_32
#elif defined(__x86_64__) || defined(_M_X64)
#   define FSTD_X86_64
#endif

// Intrinsics capabilities
#if defined(FL_X86_32) || defined(FL_X86_64)
#   define  FSTD_SSE2
#endif
