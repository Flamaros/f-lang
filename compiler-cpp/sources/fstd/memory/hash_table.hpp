#pragma once

#include <fstd/language/types.hpp>

#include <fstd/system/allocator.hpp>

#include <fstd/memory/array.hpp>

#include <fstd/core/assert.hpp>

namespace fstd
{
	namespace memory
	{
		// @TODO Check if the Key_Type is an integer type (uint16, int32, ...)
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
		template<typename Key_Type, typename Value_Type>
		struct Hash_Table
		{

		};
	}
}

// Hash functions
// http://www.cse.yorku.ca/~oz/hash.html
// djb2 seems good, but is it good enough for me?
//
// New hash32 and hash64
// http://burtleburtle.net/bob/hash/evahash.html

// http://burtleburtle.net/bob/c/lookup8.c