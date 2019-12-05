#pragma once

namespace fight_std {
	template<typename Type, size_t size>
	class Bucket
	{
		constexpr size_t	m_size = size;
		Type				m_elements[size];
	};
}
