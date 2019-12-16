#pragma once

#include <cstdint>

namespace fstd {
	namespace memory {
		struct string_ref
		{
			uint8_t*	ref_ptr;
			uint32_t	length;
			uint8_t*	original_ptr;
			uint8_t*	original_length;
		};
	}
}
