#pragma once

#if defined(_WIN32)
#	define FL_OPENGL
#   define FL_DESKTOP
#	define FL_WIN32
#	if defined(_WIN64)
#		define FL_WIN64
#   endif

#elif defined(__APPLE__)
#   define FL_APPLE
#	include "TargetConditionals.h"
#	if TARGET_OS_IPHONE == 1 || TARGET_OS_IPAD == 1 || TARGET_OS_IPOD == 1
#		define FL_IOS
#		define FL_OPENGL_ES
#       define FL_MOBILE
#       if TARGET_OS_TV == 1
#           define FL_TVOS
#       endif
#	else
#		define FL_MACOS
#		define FL_OPENGL
#       define FL_DESKTOP
#   endif

#elif defined(__ANDROID__)
#   define FL_ANDROID
#   define FL_OPENGL_ES
#   define FL_MOBILE

#elif defined(__linux__)
#   define FL_LINUX
#endif

// Compilers

#if defined(_MSC_VER)
#   define __MSVC__     _MSC_VER
#   define FL_MSVC     __MSVC__
#endif

#if defined(__GNUC__)
#	define __GCC_VERSION__        (__GNUC__ * 10000 \
	+ __GNUC_MINOR__ * 100 \
	+ __GNUC_PATCHLEVEL__)
#	define FL_GCC              __GCC_VERSION__
#endif

//#if defined(_DEBUG) || defined(DEBUG) || defined (__DEBUG__)
//#   define FL_DEBUG
//#else
//#   define FL_RELEASE
//#endif

// Architectures
#if defined(__arm__)
#   define FL_ARM
#elif defined(__i386__) || defined(_M_IX86)
#   define FL_X86_32
#elif defined(__x86_64__) || defined(_M_X64)
#   define FL_X86_64
#endif

// Intrinsics capabilities
#if defined(FL_X86_32) || defined(FL_X86_64)
#   define  FL_SSE2
#endif
