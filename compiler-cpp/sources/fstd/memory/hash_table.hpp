#pragma once

#include <fstd/language/types.hpp>

#include <fstd/system/allocator.hpp>

#include <fstd/memory/array.hpp>

#include <fstd/core/assert.hpp>
#include <fstd/core/logger.hpp>

#include <fstd/macros.hpp>

#undef max
#include <limits> // @TODO remove it

//
// Voir sur les buckets de la hashtable s'il ne faut pas avoir un buffer avec les flag d'usage plutôt que
// mettre la value en pointeur.Ça prendrait plus de mémoire, mais peut-être que c'est intéressant
// d'avoir moins d'allocations et copies memoire lors de l'insertion. Dans ce cas ça serait à l'utilisateur
// de mettre en type pointeur en paramètre template si le type de sa valeur est de grande taille
//
// Mettre des commentaires là dessus, car pas sûr que ce soit intéressant pour la hashtable où on ne fait
// pas des insertions par lot à des endroits proches en mémoire.
//
// Bien mesurer la mémoire consommée, mettre aussi les marqueurs Tracy pour ça.
//
// Comparer les perfs avec une unordered_map. Bien documenter le fait que l'itération sur l'ensemble des
// éléments risque d'être lente.
//
// 
// @ TODO il faut voir pour les collisions qui posent pb avec le remove
// !!!!!! Remplacer les Keyword_Hash_Table par cette version une fois finie

// @Warning L'implémentation se base sur le fait que les éléments ne peuvent pas être supprimés!!!


// @TODO @Warning Pour pouvoir supporter les suppressions
// Passer sur une liste chaînée pour les valeurs qui ont la même clé, ça sera utile pour le remove.
// Sinon si les buckets sont assez petits il faut peut-être faire en sorte que la gestion des collisions reste bloquée sur un bucket,
// ainsi dans le pire des cas on n'iterera jamais sur plus de slot que la taille d'un bucket (à la fois pour la suppression et pour l'insertion).
// Mais c'est peut-être moins fiable si un bucket arrive à se remplir complètement, on aura des collisions en boucle, est-ce que dans ce cas c'est pas valide
// d'avoir l'insertion qui échoue? L'utilisateur pourra soit augmenter la taille de ses buckets soit revoir la fonction de hash utilisée.
//
// Voir quand même pour éviter les copies de valeurs.
// 
// https://pvk.ca/Blog/more_numerical_experiments_in_hashing.html
// https://gist.github.com/pervognsen/564e8d54589349855d955d8bd0e57292
// https://gist.github.com/pervognsen/2470bd6e992d83bb59de11ddcd30895f


// @TODO KeywordHashTable
// Peut être continuer d'utiliser keyword hashtable mais avec un autre générateur de hash



namespace fstd
{
	namespace memory
	{
		// @TODO In f version template should be much simpler to use than in C++, especialy for functions
		// that manipulate the Hash_Table.
		// And for the user of the Hash_Table API we should have auto deduction of templates types (deduction
		// of the template version of the function by using the parameters types (the hash_table passed by
		// parameters should already contains in his typeinfo the template parameters (and their types)).

		template<typename Hash_Type, typename Key_Type, typename Value_Type, size_t _bucket_size = 512>
		struct Hash_Table
		{
			// @TODO Check if the Key_Type is an integer type (uint16, int32, ...)
			// With templates "contracts"

			struct Iterator
			{
				const Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>* hash_table;
				size_t bucket_index;
				size_t slot_index;
			};

			struct Value_POD
			{
				Key_Type	key;
				Value_Type* value;
			};

			struct Bucket
			{
				Array<Value_POD>	table;
				size_t				nb_values;	// If 0 the table have a size of 0, else the bucket_size
			};

			bool (*compare_function)(const Key_Type&, const Key_Type&) = nullptr;
			Array<Bucket> buckets;
		};

		template<typename Hash_Type, typename Key_Type, typename Value_Type, size_t _bucket_size>
		inline void hash_table_init(Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>& hash_table, bool (*compare_function)(const Key_Type&, const Key_Type&))
		{
			hash_table.compare_function = compare_function;

			fstd::core::Assert((std::numeric_limits<Hash_Type>::max() + 1) % _bucket_size == 0); // maximum possible value of type Hash_Type should be a multiple of bucket_size

			init(hash_table.buckets);
			resize_array(hash_table.buckets, (std::numeric_limits<Hash_Type>::max() + 1) / _bucket_size);

			for (size_t bucket_index = 0; bucket_index < get_array_size(hash_table.buckets); bucket_index++)
			{
				auto* bucket = get_array_element(hash_table.buckets, bucket_index);
				init(bucket->table);
				bucket->nb_values = 0;
			}

			// @Speed We have to set all values ptr to null, so we can simply use a fill_memory which is more optimal than a basic loop.
		}

		template<typename Hash_Type, typename Key_Type, typename Value_Type, size_t _bucket_size>
		inline void hash_table_release(Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>& hash_table)
		{
			for (size_t bucket_index = 0; bucket_index < get_array_size(hash_table.buckets); bucket_index++)
			{
				auto* bucket = get_array_element(hash_table.buckets, bucket_index);

				for (size_t i = 0; i < get_array_size(bucket->table); i++) // @Warning some buckets aren't allocated so the size can be 0
				{
					auto value_pod = get_array_element(bucket->table, i);
					if (value_pod->value != nullptr)
						system::free(value_pod->value);
				}
				release(bucket->table);
			}
			release(hash_table.buckets);
		}

		template<typename Hash_Type, typename Key_Type, typename Value_Type, size_t _bucket_size>
		inline void hash_table_insert(Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>& hash_table, Hash_Type hash, Key_Type& key, Value_Type& value)
		{
			fstd::core::Assert(hash_table.compare_function);

			while (true)
			{
				size_t bucket_index = hash / _bucket_size;
				size_t value_index = hash - (bucket_index * _bucket_size);
				auto* bucket = get_array_element(hash_table.buckets, bucket_index);

				if (get_array_size(bucket->table) == 0)
				{
					// We have to initialize the bucket for its first use
					init(bucket->table);
					resize_array(bucket->table, _bucket_size);
					system::fill_memory(get_array_data(bucket->table), get_array_bytes_size(bucket->table), 0x00);
				}

				auto value_pod = get_array_element(bucket->table, value_index);
				if (value_pod->value == nullptr)
				{
					value_pod->key = key;
					value_pod->value = (Value_Type*)system::allocate(sizeof(Value_Type));
					system::memory_copy(value_pod->value, &value, sizeof(Value_Type));

					bucket->nb_values++;
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

		//template<typename Hash_Type, typename Key_Type, typename Value_Type>
		//inline void hash_table_remove(Hash_Table<Hash_Type, Key_Type, Value_Type>&hash_table, Hash_Type hash, Key_Type& key)
		//{
		//	fstd::core::Assert(hash_table.compare_function);

		//	while (true)
		//	{
		//		size_t bucket_index = hash / _bucket_size;
		//		size_t value_index = hash - (bucket_index * _bucket_size);
		//		auto* bucket = get_array_element(hash_table.buckets, bucket_index);

		//		auto value_pod = get_array_element(bucket->table, value_index);
		//		// @Fuck
		//		// Comment retrouver la key quand il y a eu une collision et que la première insertion à été supprimée?
		//		// Risque de chercher une key a travers toute la hash_table qui a été supprimée. BADDDDD!!!!!
		//		if (value_pod && hash_table.compare_function(key, value_pod->key) == true)
		//		{

		//			return;
		//		}

		//		bucket->nb_values--;
		//		if (bucket->nb_values == 0)
		//		{
		//			release(bucket->table);
		//		}

		//		// Handle hash collision
		//		hash++;
		//	}
		//}

		template<typename Hash_Type, typename Key_Type, typename Value_Type, size_t _bucket_size>
		inline Value_Type* hash_table_get(Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>& hash_table, Hash_Type hash, Key_Type& key)
		{
			fstd::core::Assert(hash_table.compare_function);

			while (true)
			{
				size_t bucket_index = hash / _bucket_size;
				size_t value_index = hash - (bucket_index * _bucket_size);
				auto* bucket = get_array_element(hash_table.buckets, bucket_index);

				auto value_pod = get_array_element(bucket->table, value_index);

				if (value_pod == nullptr) // Slot is empty so the key is not in the hash_table
					return nullptr;

				if (hash_table.compare_function(key, value_pod->key) == true)
					return value_pod->value;
					
				// Increment the hash due to the collision, but we also take care of keeping it in the range.
				hash = (hash + 1) % std::numeric_limits<Hash_Type>::max();
			}
		}

		template<typename Hash_Type, typename Key_Type, typename Value_Type, size_t _bucket_size>
		inline Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>::Iterator hash_table_begin(const Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>& hash_table)
		{
			// We search the first valid value

			typename Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>::Iterator it;

			it.hash_table = &hash_table;

			for (it.bucket_index = 0; it.bucket_index < get_array_size(hash_table.buckets); it.bucket_index++)
			{
				auto* bucket = get_array_element(hash_table.buckets, it.bucket_index);

				for (it.slot_index = 0; it.slot_index < get_array_size(bucket->table); it.slot_index++) // @Warning some buckets aren't allocated so the size can be 0
				{
					auto value_pod = get_array_element(bucket->table, it.slot_index);
					if (value_pod->value != nullptr)
						return it;
				}
			}

			return it;
		}

		template<typename Hash_Type, typename Key_Type, typename Value_Type, size_t _bucket_size>
		inline Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>::Iterator& hash_table_next(typename Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>::Iterator& it)
		{
			// We increment the slot index to start to test the next value and we keep incrementing until a value is found

			it.slot_index++;

			for (; it.bucket_index < get_array_size(it.hash_table->buckets); it.bucket_index++)
			{
				auto* bucket = get_array_element(it.hash_table->buckets, it.bucket_index);

				for (; it.slot_index < get_array_size(bucket->table); it.slot_index++) // @Warning some buckets aren't allocated so the size can be 0
				{
					auto value_pod = get_array_element(bucket->table, it.slot_index);
					if (value_pod->value != nullptr)
						return it;
				}
				it.slot_index = 0;
			}

			return it;
		}

		template<typename Hash_Type, typename Key_Type, typename Value_Type, size_t _bucket_size>
		inline Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>::Iterator hash_table_end(const Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>& hash_table)
		{
			typename Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>::Iterator it;

			it.hash_table = &hash_table;

			it.bucket_index = get_array_size(hash_table.buckets);
			it.slot_index = 0; // Not _bucket_size because when we go after the last bucket the slot_index is reset to 0 (take a look at hash_table_next implementation)

			return it;
		}

		template<typename Hash_Type, typename Key_Type, typename Value_Type, size_t _bucket_size>
		inline bool equals(typename Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>::Iterator& a, typename Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>::Iterator& b)
		{
			return a.hash_table == b.hash_table && a.bucket_index == b.bucket_index && a.slot_index == b.slot_index;
		}

		template<typename Hash_Type, typename Key_Type, typename Value_Type, size_t _bucket_size>
		inline Value_Type* hash_table_get(typename Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>::Iterator& it)
		{
			auto* bucket = get_array_element(it.hash_table->buckets, it.bucket_index);

			if (bucket == nullptr)
				return nullptr;

			auto value_pod = get_array_element(bucket->table, it.slot_index);

			if (value_pod == nullptr) // Slot is empty so the key is not in the hash_table
				return nullptr;

			return value_pod->value;
		}

		template<typename Hash_Type, typename Key_Type, typename Value_Type, size_t _bucket_size>
		inline void log_stats(Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>& hash_table, fstd::core::Logger* logger)
		{
			fstd::core::log(logger, fstd::core::Log_Level::info, "[Hash_Table] Memory size: %d\n", get_array_bytes_size(hash_table.table));
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