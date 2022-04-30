#pragma once

#include <fstd/system/allocator.hpp>

#include <fstd/core/assert.hpp>

namespace fstd
{
	namespace memory
	{
		template<typename Type>
		struct Array
		{
			Type*	ptr = nullptr;
			size_t	reserved = 0;
			size_t	size = 0;				// Number of elements

											// @TODO @CleanUp
			// I don't really want to put the operator here (directly in the struct)
			inline Type& operator[](size_t index)
			{
				return ptr[index];
			}

			// @TODO @WTF??? Why I need a const version???
			inline const Type& operator[](size_t index) const
			{
				return ptr[index];
			}
		};

		template<typename Type>
		void init(Array<Type>& array)
		{
			array.ptr = nullptr;
			array.reserved = 0;
			array.size = 0;
		}

		template<typename Type>
		void resize_array(Array<Type>& array, size_t size)
		{
			if (array.reserved >= size) {
				array.size = size;
			}
			else {
				array.ptr = (Type*)system::reallocate(array.ptr, size * sizeof(Type));
				array.reserved = size;
				array.size = size;
			}
		}

		template<typename Type>
		void reserve_array(Array<Type>& array, size_t size)
		{
			if (array.reserved < size) {
				array.ptr = (Type*)system::reallocate(array.ptr, size * sizeof(Type));
				array.reserved = size;
			}
		}

		template<typename Type>
		void release(Array<Type>& array)
		{
			system::free(array.ptr);
			init(array);
		}

		template<typename Type>
		void shrink_array(Array<Type>& array, size_t size)
		{
			if (size == 0) {
				release(array);

			}
			else if (array.reserved > array.size) {
				Type* old_buffer = array.ptr;

				array.ptr = system::allocate(array.ptr, array.size * sizeof(Type));
				system::memory_copy(array.ptr, old_buffer, array.size * sizeof(Type));
				array.reserved = size;

				system::free(old_buffer);
			}
		}

		template<typename Type>
		void array_push_back(Array<Type>& array, Type value)
		{
			reserve_array(array, array.size + 1);
			array.ptr[array.size] = value;
			array.size++;
		}

		template<typename Type>
		void array_copy(Array<Type>& array, size_t index, const Type* raw_array, size_t size)
		{
			resize_array(array, index + size);
			system::memory_copy(&array.ptr[index], raw_array, size * sizeof(Type));
		}

		template<typename Type>
		void array_copy(Array<Type>& array, size_t index, const Array<Type>& source)
		{
			resize_array(array, index + source.size);
			system::memory_copy(&array.ptr[index], source.ptr, source.size * sizeof(Type));
		}

		template<typename Type>
		Type* get_array_element(const Array<Type>& array, size_t index)
		{
			fstd::core::Assert(array.reserved > index);
			return &array.ptr[index];
		}

		template<typename Type>
		Type* get_array_first_element(const Array<Type>& array)
		{
			Assert(array.size);
			return &array.ptr[0];
		}

		template<typename Type>
		Type* get_array_last_element(const Array<Type>& array)
		{
			fstd::core::Assert(array.size);
			return &array.ptr[array.size - 1];
		}

		template<typename Type>
		Type* get_array_data(const Array<Type>& array)
		{
			return array.ptr;
		}

		template<typename Type>
		size_t get_array_size(const Array<Type>& array)
		{
			return array.size;
		}

		template<typename Type>
		size_t get_array_bytes_size(const Array<Type>& array)
		{
			return array.size * sizeof(Type);
		}

		template<typename Type>
		size_t is_array_empty(const Array<Type>& array)
		{
			return array.size == 0;
		}

		template<typename Type>
		size_t get_array_reserved(const Array<Type>& array)
		{
			return array.reserved;
		}
	}
}
