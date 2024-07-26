#pragma once

#include <fstd/system/allocator.hpp>

#include <fstd/core/assert.hpp>

#include <tracy/Tracy.hpp>

namespace fstd
{
	namespace memory
	{
		template<typename Type>
		struct Array
		{
			Type*	ptr = nullptr;
			ssize_t	reserved = 0;
			ssize_t	size = 0;				// Number of elements

											// @TODO @CleanUp
			// I don't really want to put the operator here (directly in the struct)
			inline Type& operator[](ssize_t index)
			{
				return ptr[index];
			}

			// @TODO @WTF??? Why I need a const version???
			inline const Type& operator[](ssize_t index) const
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
		void resize_array(Array<Type>& array, ssize_t size)
		{
			ZoneScopedN("resize_array");

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
		void reserve_array(Array<Type>& array, ssize_t size)
		{
			ZoneScopedN("reserve_array");

			if (array.reserved < size) {
				array.ptr = (Type*)system::reallocate(array.ptr, size * sizeof(Type));
				array.reserved = size;
			}
		}

		template<typename Type>
		void release(Array<Type>& array)
		{
			ZoneScopedN("release");

			system::free(array.ptr);
			init(array);
		}

		template<typename Type>
		void shrink_array(Array<Type>& array, ssize_t size)
		{
			ZoneScopedN("shrink_array");

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
		void array_push_back(Array<Type>& array, const Type& value)
		{
			ZoneScopedN("array_push_back");

			reserve_array(array, array.size + 1);
			array.ptr[array.size] = value;
			array.size++;
		}

		template<typename Type>
		void array_copy(Array<Type>& array, ssize_t index, const Type* raw_array, ssize_t size)
		{
			ZoneScopedN("array_copy");

			resize_array(array, index + size);
			system::memory_copy(&array.ptr[index], raw_array, size * sizeof(Type));
		}

		template<typename Type>
		void array_copy(Array<Type>& array, ssize_t index, const Array<Type>& source, ssize_t start_index = 0, ssize_t size = -1)
		{
			ZoneScopedN("array_copy");

			if (size == -1) {
				size = source.size - start_index;
			}

			fstd::core::Assert(
				start_index >= 0 &&
				start_index < source.size &&
				size > 0 &&	// We should copy at least one element
				start_index + size <= source.size);

			resize_array(array, index + size);
			system::memory_copy(&array.ptr[index], &source.ptr[start_index], size * sizeof(Type));
		}

		template<typename Type>
		Type* get_array_element(const Array<Type>& array, ssize_t index)
		{
			fstd::core::Assert(array.size > index);
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
		ssize_t get_array_size(const Array<Type>& array)
		{
			return array.size;
		}

		template<typename Type>
		ssize_t get_array_bytes_size(const Array<Type>& array)
		{
			return array.size * sizeof(Type);
		}

		template<typename Type>
		ssize_t is_array_empty(const Array<Type>& array)
		{
			return array.size == 0;
		}

		template<typename Type>
		ssize_t get_array_reserved(const Array<Type>& array)
		{
			return array.reserved;
		}

		// @SpeedUp Est-ce que la fonction de matching doit être en paramètre template afin de pouvoir être inlinée?
		// Car avec un pointeur sur fonction je ne suis pas sur que le compilateur ou linker soient en mesure de supprimer le call
		// Le pb c'est qu'en C++ pour être sur que le paramètre template soit une fonction qui respecte le bon prototype est over complicated.
		template<typename Type, typename SearchType>
		ssize_t find_array_element(const Array<Type>& array, const SearchType& value, bool (*match_function)(const SearchType&, const Type&), ssize_t start_index = 0)
		{
			for (ssize_t i = start_index; i < array.size; i++)
			{
				if (match_function(value, array[i])) {
					return i;
				}
			}
			return -1;
		}

	}
}
