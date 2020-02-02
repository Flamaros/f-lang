#pragma once

#include <fstd/language/types.hpp>

namespace fstd {
	namespace language {
		struct string_view
		{
			const uint8_t*	ptr = nullptr;
			size_t			length = 0;
		};

		inline void assign(string_view& str, const uint8_t* start, size_t length)
		{
			str.ptr = start;
			str.length = length;
		}

		inline void resize(string_view& str, size_t length)
		{
			str.length = length;
		}

		inline size_t get_string_length(const string_view& str)
		{
			return str.length;
		}

		inline const uint8_t* get_data(const string_view& str)
		{
			return str.ptr;
		}
	}
}
