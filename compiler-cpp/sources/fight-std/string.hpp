#pragma once

#include <cstdint>

namespace fight_std {
    struct string
    {
        uint8_t*    data;
        uint32_t    length;
    };

	inline void	initialize(string& str)
	{
		str.data = nullptr;
		str.length = 0;
	}

	size_t	strlen(const char* buffer)
	{
		size_t	len = 0;

		if (buffer == nullptr) {
			return 0;
		}

		for (; buffer[len] != '\0'; len++)
			;

		return len;
	}

	void assign(string& str, const char* buffer)
	{
		str.data = (uint8_t*)buffer;
		str.length = strlen(buffer);
	}
}
