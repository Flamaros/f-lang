#pragma once

#include <fstd/platform.hpp>

typedef signed char        int8_t;
typedef short              int16_t;
typedef int                int32_t;
typedef long long          int64_t;
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

#if defined(FSTD_X86_64)

typedef uint64_t size_t;

#elif defined(FSTD_X86_32)

typedef uint32_t size_t;

#else
#	error
#endif
