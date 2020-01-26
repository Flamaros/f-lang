#pragma once

#include <fstd/language/types.hpp>

namespace fstd
{
	namespace memory
	{
		template<typename Type, size_t capacity>
		class Bucket
		{
			constexpr size_t	size = size;
			Type				m_elements[capacity];
			size_t				m_size;
		};
	}
}
