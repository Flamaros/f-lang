#pragma once

#include <fstd/language/types.hpp>

namespace fstd
{
	namespace memory
	{
		template<typename Type, size_t bucket_size = 512>
		struct Bucket_Array
		{
			Type**	ptr = nullptr;		// An array of buckets
			size_t	size = 0;			// Number of elements
		};

		template<typename Type, size_t bucket_size>
		inline void init(Bucket_Array<Type, bucket_size>& array)
		{
			array.ptr = nullptr;
			array.size = 0;
		}

		template<typename Type, size_t bucket_size>
		inline void push_back(Bucket_Array<Type, bucket_size>& array, const Type* data, size_t size)
		{
			// @TODO
			// Il faut faire attention à la copie à cheval entre deux buckets
		}

		template<typename Type, size_t bucket_size>
		inline void push_back(Bucket_Array<Type, bucket_size>& array, Type& value)
		{
			// @TODO
			// Créer un nouveau bucket si besoin
		}
	}
}
