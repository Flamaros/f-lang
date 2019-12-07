#pragma once

namespace fstd
{
	namespace memory
	{
		template<typename Type, size_t size>
		class Bucket
		{
			constexpr size_t	size = size;
			Type				m_elements[size];
		};
	}
}
