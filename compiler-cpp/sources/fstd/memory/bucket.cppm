export module fstd.memory.bucket;

import fstd.language.types;

namespace fstd
{
	namespace memory
	{
		export
		template<typename Type, size_t capacity>
		class Bucket
		{
			static constexpr size_t	m_capacity = capacity;
			Type					m_elements[capacity];
			size_t					m_size;
		};
	}
}
