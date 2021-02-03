#pragma once

#include "array.hpp"

#include <fstd/system/allocator.hpp>

#include <fstd/core/assert.hpp>

namespace fstd
{
	namespace memory
	{
		template<typename Type>
		struct Stack
		{
			Array<Type>	array;
		};

		template<typename Type>
		void init(Stack<Type>& stack)
		{
			init(stack.array);
		}

		template<typename Type>
		void stack_push(Stack<Type>& stack, Type value)
		{
			array_push_back(stack.array, value);
		}

		template<typename Type>
		Type stack_pop(Stack<Type>& stack)
		{
			Type	value;
			size_t	size = get_array_size(stack.array);

			core::Assert(size != 0);

			value = *get_array_last_element(stack.array);
			resize_array(stack.array, size - 1);
			return value;
		}

		template<typename Type>
		size_t get_stack_size(const Stack<Type>& stack)
		{
			return get_array_size(stack.array);
		}

		template<typename Type>
		Type* get_stack_current_value(const Stack<Type>& stack)
		{
			Type	value;
			size_t	size = get_array_size(stack.array);

			core::Assert(size != 0);

			return get_array_last_element(stack.array);
		}
	}
}
