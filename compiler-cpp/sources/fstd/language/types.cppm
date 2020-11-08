export module fstd.language.types;

#include "fstd/platform.hpp"

export typedef char					int8_t;
export typedef short				int16_t;
export typedef int					int32_t;
export typedef long long			int64_t;
export typedef unsigned char		uint8_t;
export typedef unsigned short		uint16_t;
export typedef unsigned int			uint32_t;
export typedef unsigned long long	uint64_t;

#if defined(FSTD_X86_64)

export typedef uint64_t size_t;

#elif defined(FSTD_X86_32)

export typedef uint32_t size_t;

#else
#	error
#endif
