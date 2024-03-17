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
		inline void push_back(Bucket_Array<Type, bucket_size>& array, Type& value)
		{
			// @TODO
			// Créer un nouveau bucket si besoin
		}

		template<typename Type, size_t bucket_size>
		Type* get_array_element(const Bucket_Array<Type, bucket_size>& array, size_t index)
		{
			fstd::core::Assert(array.size > index);
			// @TODO
			return nullptr;
		}

		template<typename Type, size_t bucket_size>
		size_t get_array_size(const Bucket_Array<Type, bucket_size>& array)
		{
			return array.size;
		}

		template<typename Type, size_t bucket_size>
		size_t get_array_bytes_size(const Bucket_Array<Type, bucket_size>& array)
		{
			return array.size * sizeof(Type);
		}

		template<typename Type, size_t bucket_size>
		size_t is_array_empty(const Bucket_Array<Type, bucket_size>& array)
		{
			return array.size == 0;
		}

		// @TODO add an iterator over buckets for fast copy (or write to files)
	}
}
