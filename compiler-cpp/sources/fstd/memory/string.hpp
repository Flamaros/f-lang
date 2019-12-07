#pragma once

#include <string>

#include <cstdint>

namespace fstd {
	namespace memory
	{
		struct String
		{
			uint8_t*	data = nullptr;
			size_t		length = 0;
		};

		//void assign(String& str, const std::string& string)
		//{
		//	str.data = (uint8_t*)string.c_str();
		//	str.length = string.length();
		//}
	}
}
