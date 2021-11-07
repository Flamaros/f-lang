#pragma once

#include <fstd/language/types.hpp>

#include <fstd/system/allocator.hpp>

#include <fstd/memory/array.hpp>

#include <fstd/core/assert.hpp>
#include <fstd/core/logger.hpp>

#include <fstd/macros.hpp>

#undef max
#include <limits> // @TODO remove it

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

		// @TODO In f version template should be much simpler to use than in C++, especialy for functions
		// that manipulate the Hash_Table.
		// And for the user of the Hash_Table API we should have auto deduction of templates types (deduction
		// of the template version of the function by using the parameters types (the hash_table passed by
		// parameters should already contains in his typeinfo the template parameters (and their types)).
		template<typename Hash_Type, typename Key_Type, typename Value_Type>
		struct Hash_Table
		{
			struct Value_POD
			{
				Key_Type key;
				Value_Type* value;
			};

			bool (*compare_function)(const Key_Type&, const Key_Type&) = nullptr;
			Array<Value_POD> table;
		};

		template<typename Hash_Type, typename Key_Type, typename Value_Type>
		inline void hash_table_init(Hash_Table<Hash_Type, Key_Type, Value_Type>& hash_table, bool (*compare_function)(const Key_Type&, const Key_Type&))
		{
			hash_table.compare_function = compare_function;

			resize_array(hash_table.table, std::numeric_limits<Hash_Type>::max());

			// @Speed We have to set all values ptr to null, so we can simply use a fill_memory which is more optimal than a basic loop.
			system::fill_memory(get_array_data(hash_table.table), get_array_bytes_size(hash_table.table), 0x00);

#if defined(FSTD_DEBUG)
			fstd::core::log(*globals.logger, fstd::core::Log_Level::info, "[Hash_Table] Memory size: %d\n", get_array_bytes_size(hash_table.table));
#endif
		}

		template<typename Hash_Type, typename Key_Type, typename Value_Type>
		inline void hash_table_release(Hash_Table<Hash_Type, Key_Type, Value_Type>& hash_table)
		{
			// @TODO handle buckets

			for (Hash_Type i = 0; i != std::numeric_limits<Hash_Type>::max(); i++)
			{
				auto value_pod = get_array_element(hash_table.table, i);
				if (value_pod->value != nullptr)
					system::free(value_pod->value);
			}
			release(hash_table.table);
		}

		template<typename Hash_Type, typename Key_Type, typename Value_Type>
		inline void hash_table_insert(Hash_Table<Hash_Type, Key_Type, Value_Type>& hash_table, Hash_Type hash, Key_Type& key, Value_Type& value)
		{
			fstd::core::Assert(hash_table.compare_function);

			// @TODO handle buckets
			// Initialization of buckets (check hash_table_init with values initilization)

			while (true)
			{
				auto value_pod = get_array_element(hash_table.table, hash);
				if (value_pod->value == nullptr)
				{
					value_pod->key = key;
					value_pod->value = (Value_Type*)system::allocate(sizeof(Value_Type));
					system::memory_copy(value_pod->value, &value, sizeof(Value_Type));
					return;
				}
				else if (hash_table.compare_function(key, value_pod->key) == true)
				{
					// This value already exist
					return;
				}

				// Handle hash collision
				hash++;
			}
		}

		template<typename Hash_Type, typename Key_Type, typename Value_Type>
		inline Value_Type* hash_table_get(Hash_Table<Hash_Type, Key_Type, Value_Type>& hash_table, Hash_Type hash, Key_Type& key)
		{
			fstd::core::Assert(hash_table.compare_function);

			while (true)
			{
				auto value_pod = get_array_element(hash_table.table, hash);
				if (hash_table.compare_function(key, value_pod->key) == true)
					return value_pod->value;
					
				hash++;
			}
			// @TODO we need the hash to get the index and the key to do a deep compare to be sure to have the right value
		}
	}
}

// Hash functions
// http://www.cse.yorku.ca/~oz/hash.html
// djb2 seems good, but is it good enough for me?
//
// New hash32 and hash64
// http://burtleburtle.net/bob/hash/evahash.html

// http://burtleburtle.net/bob/c/lookup8.c