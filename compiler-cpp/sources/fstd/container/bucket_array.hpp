#pragma once

#include <fstd/language/types.hpp>
#include <fstd/math/math.hpp>

namespace fstd
{
	namespace container
	{
		template<typename Type, ssize_t bucket_size = 512>
		struct Bucket_Array
		{
			Type**	ptr = nullptr;		// An array of buckets
			ssize_t	size = 0;			// Number of elements
		};

		template<typename Type, ssize_t bucket_size>
		inline void init(Bucket_Array<Type, bucket_size>& array)
		{
			array.ptr = nullptr;
			array.size = 0;
		}

		template<typename Type, ssize_t bucket_size>
		inline constexpr ssize_t get_bucket_size(const Bucket_Array<Type, bucket_size>& array)
		{
			return bucket_size;
		}

		template<typename Type, ssize_t bucket_size>
		inline void push_back(Bucket_Array<Type, bucket_size>& array, Type& value)
		{
			push_back(array, &value, 1);
		}

		template<typename Type, ssize_t bucket_size>
		inline void push_back(Bucket_Array<Type, bucket_size>& array, Type* value, ssize_t count)
		{
			core::Assert(count > 0);

			ssize_t last_bucket_index = array.size / bucket_size;
			ssize_t nb_allocated_buckets = array.ptr ? last_bucket_index + 1 : 0;
			ssize_t nb_needed_buckets = count / bucket_size + 1;
			ssize_t starting_index_in_bucket = array.size % bucket_size;

			if (nb_needed_buckets > nb_allocated_buckets) {
				array.ptr = (Type**)system::reallocate(array.ptr, nb_needed_buckets * sizeof(Type*));
				for (ssize_t i = last_bucket_index; i < nb_needed_buckets; i++) {
					array.ptr[i] = (Type*)system::allocate(bucket_size * sizeof(Type));
				}
			}

			ssize_t copied_count = 0;
			for (ssize_t i = last_bucket_index; i < nb_needed_buckets; i++) {
				ssize_t count_to_copy = math::min(count - copied_count, bucket_size - starting_index_in_bucket);
				system::memory_copy(&array.ptr[i][starting_index_in_bucket],
					&value[copied_count], count_to_copy * sizeof(Type));
				copied_count += count_to_copy;

				starting_index_in_bucket = 0;
			}

			array.size += count;
		}

		template<typename Type, ssize_t bucket_size>
		Type* get_bucket(const Bucket_Array<Type, bucket_size>& array, ssize_t bucket_index)
		{
			ssize_t last_bucket_index = array.size / bucket_size;
			core::Assert(bucket_index <= last_bucket_index);

			return array.ptr[bucket_index];
		}

		template<typename Type, ssize_t bucket_size>
		Type* get_array_element(const Bucket_Array<Type, bucket_size>& array, ssize_t index)
		{
			fstd::core::Assert(array.size > index);

			ssize_t bucket_index = index / bucket_size;
			ssize_t index_in_bucket = index % bucket_size;

			return &array.ptr[bucket_index][index_in_bucket];
		}

		template<typename Type, ssize_t bucket_size>
		ssize_t get_array_size(const Bucket_Array<Type, bucket_size>& array)
		{
			return array.size;
		}

		template<typename Type, ssize_t bucket_size>
		ssize_t get_array_bytes_size(const Bucket_Array<Type, bucket_size>& array)
		{
			return array.size * sizeof(Type);
		}

		template<typename Type, ssize_t bucket_size>
		ssize_t is_array_empty(const Bucket_Array<Type, bucket_size>& array)
		{
			return array.size == 0;
		}

		// Only buckets are released not the main array (array of buckets)
		template<typename Type, ssize_t bucket_size>
		void reset_array(Bucket_Array<Type, bucket_size>& array)
		{
			ZoneScopedN("reset");

			ssize_t last_bucket_index = array.size / bucket_size;
			ssize_t nb_allocated_buckets = array.ptr ? last_bucket_index + 1 : 0;

			for (ssize_t i = 0; i < nb_allocated_buckets; i++) {
				system::free(array.ptr[i]);
			}

			array.size = 0;
		}

		// @TODO add an iterator over buckets for fast copy (or write to files)
	}
}
