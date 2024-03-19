#pragma once

#include <fstd/language/types.hpp>

#include <algorithm> // Just for std::min old C macros are....

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
			push_back(array, &value, 1);
		}

		template<typename Type, size_t bucket_size>
		inline void push_back(Bucket_Array<Type, bucket_size>& array, Type* value, size_t count)
		{
			core::Assert(count > 0);

			size_t last_bucket_index = array.size / bucket_size;
			size_t nb_allocated_buckets = array.ptr ? last_bucket_index + 1 : 0;
			size_t nb_needed_buckets = count / bucket_size + 1;
			size_t starting_index_in_bucket = array.size % bucket_size;

			if (nb_needed_buckets > nb_allocated_buckets) {
				array.ptr = (Type**)system::reallocate(array.ptr, nb_needed_buckets * sizeof(Type*));
				for (size_t i = last_bucket_index; i < nb_needed_buckets; i++) {
					array.ptr[i] = (Type*)system::allocate(bucket_size * sizeof(Type));
				}
			}

			size_t copied_count = 0;
			for (size_t i = last_bucket_index; i < nb_needed_buckets; i++) {
				size_t count_to_copy = std::min(count - copied_count, bucket_size - starting_index_in_bucket);
				system::memory_copy(&array.ptr[i][starting_index_in_bucket],
					&value[copied_count], count_to_copy * sizeof(Type));
				copied_count += count_to_copy;

				starting_index_in_bucket = 0;
			}

			array.size += count;
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
