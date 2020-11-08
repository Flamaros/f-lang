export module fstd.memory.hash_table;

import fstd.language.types;

import fstd.system.allocator;

import fstd.memory.array;

import fstd.core.assert;

namespace fstd
{
	namespace memory
	{
		// @TODO Check if the Key_Type is an integer type (uint16, int32, ...)
		export
		template<typename Key_Type, typename Key_Value_Type, typename Value_Type, size_t bucket_size>
		struct Hash_Table_S
		{
			struct Value_POD
			{

			};

			template<typename Type, size_t _capacity>
			class Bucket
			{
				size_t	capacity = _capacity;
				Type	m_elements[_capacity];
			};

			Array<Bucket<Value_POD, bucket_size>>	buckets;
		};

		// @TODO Check if the Key_Type is an integer type (uint16, int32, ...)
		export
		template<typename Key_Type, typename Value_Type>
		struct Hash_Table
		{

		};
	}
}
