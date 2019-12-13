#pragma once

#include <fstd/system/allocator.hpp>

namespace fstd
{
	namespace memory
	{
		template<typename Type>
		struct Array
		{
			Type*	buffer = nullptr;
			size_t	reserved = 0;
			size_t	size = 0;				// Number of elements
		};

		template<typename Type>
		void resize_array(Array<Type>& array, size_t size)
		{
			if (array.reserved >= size) {
				array.size = size;
			}
			else {
				array.buffer = (Type*)system::reallocate(array.buffer, size * sizeof(Type));
				array.reserved = size;
				array.size = size;
			}
		}

		template<typename Type>
		void reset_array(Array<Type>& array)
		{
			system::free(array.buffer);
			array.buffer = nullptr;
			array.reserved = 0;
			array.size = 0;
		}

		template<typename Type>
		void shrink_array(Array<Type>& array, size_t size)
		{
			if (size == 0) {
				reset_array(array);

			}
			else if (array.reserved > array.size) {
				Type* old_buffer = array.buffer;

				array.buffer = system::allocate(array.buffer, array.size * sizeof(Type));
				system::memory_copy(array.buffer, old_buffer, array.size * sizeof(Type));
				array.reserved = size;

				system::free(old_buffer);
			}
		}

		template<typename Type>
		void array_push_back(Array<Type>& array, Type value)
		{
			reserve_array(array, array.size + 1);
			array.buffer[array.size] = value;
			array.size++;
		}

		template<typename Type>
		Type* array_get(Array<Type>& array, size_t index)
		{
			return &array.buffer[index];
		}

		template<typename Type>
		Type* get_array_data(Array<Type>& array)
		{
			return array.buffer;
		}

		template<typename Type>
		size_t get_array_size(Array<Type>& array)
		{
			return array.size;
		}

		template<typename Type>
		size_t is_array_empty(Array<Type>& array)
		{
			return array.size == 0;
		}

		template<typename Type>
		size_t get_array_reserved(Array<Type>& array)
		{
			return array.reserved;
		}
	}
}
