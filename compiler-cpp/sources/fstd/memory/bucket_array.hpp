#pragma once

#include <fstd/language/types.hpp>
#include <fstd/math/math.hpp>

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
		inline constexpr size_t get_bucket_size(const Bucket_Array<Type, bucket_size>& array)
		{
			return bucket_size;
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
				size_t count_to_copy = math::min(count - copied_count, bucket_size - starting_index_in_bucket);
				system::memory_copy(&array.ptr[i][starting_index_in_bucket],
					&value[copied_count], count_to_copy * sizeof(Type));
				copied_count += count_to_copy;

				starting_index_in_bucket = 0;
			}

			array.size += count;
		}

		template<typename Type, size_t bucket_size>
		Type* get_bucket(const Bucket_Array<Type, bucket_size>& array, size_t bucket_index)
		{
			size_t last_bucket_index = array.size / bucket_size;
			core::Assert(bucket_index <= last_bucket_index);

			return array.ptr[bucket_index];
		}

		template<typename Type, size_t bucket_size>
		Type* get_array_element(const Bucket_Array<Type, bucket_size>& array, size_t index)
		{
			fstd::core::Assert(array.size > index);

			size_t bucket_index = index / bucket_size;
			size_t index_in_bucket = index % bucket_size;

			return &array.ptr[bucket_index][index_in_bucket];
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
