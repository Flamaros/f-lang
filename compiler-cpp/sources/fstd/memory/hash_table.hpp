#pragma once

#include <fstd/language/types.hpp>

#include <fstd/system/allocator.hpp>

#include <fstd/memory/array.hpp>
#include <fstd/memory/boolean_array.hpp>

#include <fstd/core/assert.hpp>
#include <fstd/core/logger.hpp>

#include <fstd/macros.hpp>

#include <limits> // @TODO remove it
#include <type_traits>

// @Warning don't use it with an hash type bigger than uint32_t

//
// Voir sur les buckets de la hashtable s'il ne faut pas avoir un buffer avec les flag d'usage plut�t que
// mettre la value en pointeur.�a prendrait plus de m�moire, mais peut-�tre que c'est int�ressant
// d'avoir moins d'allocations et copies memoire lors de l'insertion. Dans ce cas �a serait � l'utilisateur
// de mettre en type pointeur en param�tre template si le type de sa valeur est de grande taille
//
// Mettre des commentaires l� dessus, car pas s�r que ce soit int�ressant pour la hashtable o� on ne fait
// pas des insertions par lot � des endroits proches en m�moire.
//
// Bien mesurer la m�moire consomm�e, mettre aussi les marqueurs Tracy pour �a.
//
// Comparer les perfs avec une unordered_map. Bien documenter le fait que l'it�ration sur l'ensemble des
// �l�ments risque d'�tre lente.
//
// 
// @ TODO il faut voir pour les collisions qui posent pb avec le remove
// !!!!!! Remplacer les Keyword_Hash_Table par cette version une fois finie

// @Warning L'impl�mentation se base sur le fait que les �l�ments ne peuvent pas �tre supprim�s!!!


// @TODO @Warning Pour pouvoir supporter les suppressions
// Passer sur une liste cha�n�e pour les valeurs qui ont la m�me cl�, �a sera utile pour le remove.
// Sinon si les buckets sont assez petits il faut peut-�tre faire en sorte que la gestion des collisions reste bloqu�e sur un bucket,
// ainsi dans le pire des cas on n'iterera jamais sur plus de slot que la taille d'un bucket (� la fois pour la suppression et pour l'insertion).
// Mais c'est peut-�tre moins fiable si un bucket arrive � se remplir compl�tement, on aura des collisions en boucle, est-ce que dans ce cas c'est pas valide
// d'avoir l'insertion qui �choue? L'utilisateur pourra soit augmenter la taille de ses buckets soit revoir la fonction de hash utilis�e.
//
// Voir quand m�me pour �viter les copies de valeurs.
// 
// https://pvk.ca/Blog/more_numerical_experiments_in_hashing.html
// https://gist.github.com/pervognsen/564e8d54589349855d955d8bd0e57292
// https://gist.github.com/pervognsen/2470bd6e992d83bb59de11ddcd30895f


// @TODO KeywordHashTable
// Peut �tre continuer d'utiliser keyword hashtable mais avec un autre g�n�rateur de hash

#include <tracy/Tracy.hpp>

namespace fstd
{
	namespace memory
	{
		// @TODO In f version template should be much simpler to use than in C++, especialy for functions
		// that manipulate the Hash_Table.
		// And for the user of the Hash_Table API we should have auto deduction of templates types (deduction
		// of the template version of the function by using the parameters types (the hash_table passed by
		// parameters should already contains in his typeinfo the template parameters (and their types)).

		template<typename Hash_Type, typename Key_Type, typename Value_Type, uint16_t _bucket_size = 512>
		struct Hash_Table
		{
			static_assert(sizeof(Value_Type) <= sizeof(size_t));
			static_assert(std::is_integral<Hash_Type>::value, "Hash_Type should be an integral type.");

			struct Iterator
			{
				const Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>* hash_table;
				size_t bucket_index;
				size_t slot_index;
			};

			struct Value_POD
			{
				Key_Type	key;
				Value_Type	value;
			};

			// @TODO @MemOpt
			// I certainly should optimize the size of the Bucket struct to allow usage of small bucket size like 32
			// the Array<Value_POD> certainly should be a raw ptr
			struct Bucket
			{
				uint16_t					nb_values;	// If 0 the buckent is not yet initialized
				Value_POD*					table;
				Boolean_Array<_bucket_size>	init_flags;
			};

			bool (*compare_function)(const Key_Type&, const Key_Type&) = nullptr;
			Array<Bucket> buckets;
			size_t size;
		};

		template<typename Hash_Type, typename Key_Type, typename Value_Type, size_t _bucket_size>
		inline void hash_table_init(Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>& hash_table, bool (*compare_function)(const Key_Type&, const Key_Type&))
		{
			ZoneScopedN("hash_table_init");

			hash_table.compare_function = compare_function;

			fstd::core::Assert(((uint64_t)std::numeric_limits<Hash_Type>::max() + 1) % _bucket_size == 0); // maximum possible value of type Hash_Type should be a multiple of bucket_size

			init(hash_table.buckets);
			resize_array(hash_table.buckets, ((uint64_t)std::numeric_limits<Hash_Type>::max() + 1) / _bucket_size);
			system::zero_memory(get_array_element(hash_table.buckets, 0), get_array_bytes_size(hash_table.buckets));

			hash_table.size = 0;
		}

		template<typename Hash_Type, typename Key_Type, typename Value_Type, size_t _bucket_size>
		inline void hash_table_release(Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>& hash_table)
		{
			ZoneScopedN("hash_table_release");

			for (size_t bucket_index = 0; bucket_index < get_array_size(hash_table.buckets); bucket_index++)
			{
				auto* bucket = get_array_element(hash_table.buckets, bucket_index);

				system::free(bucket->table);
				release(bucket->init_flags);
			}
			release(hash_table.buckets);
		}

		template<typename Hash_Type, typename Key_Type, typename Value_Type, size_t _bucket_size>
		inline Value_Type* hash_table_insert(Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>& hash_table, Hash_Type hash, Key_Type& key, Value_Type& value)
		{
			ZoneScopedN("hash_table_insert");

			fstd::core::Assert(hash_table.compare_function);

			while (true)
			{
				size_t bucket_index = hash / _bucket_size;
				size_t value_index = hash - (bucket_index * _bucket_size);
				auto* bucket = get_array_element(hash_table.buckets, bucket_index);

				if (bucket->nb_values == 0)
				{
					ZoneScopedN("init bucket");

					// We have to initialize the bucket for its first use
					init(bucket->init_flags);
					allocate(bucket->init_flags);

					size_t bucket_size_in_bytes = _bucket_size * sizeof(Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>::Value_POD);
					bucket->table = (typename Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>::Value_POD*)system::allocate(bucket_size_in_bytes);
					system::zero_memory(bucket->table, bucket_size_in_bytes);
				}

				auto& value_pod = bucket->table[value_index];

				if (boolean_array_get(bucket->init_flags, value_index) == false)
				{
					value_pod.key = key;
					value_pod.value = value;

					hash_table.size++;
					bucket->nb_values++;
					boolean_array_set(bucket->init_flags, value_index, true);
					return &value_pod.value;
				}
				else if (hash_table.compare_function(key, value_pod.key) == true)
				{
					// This value already exist
					return &value_pod.value;
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
		//		// Comment retrouver la key quand il y a eu une collision et que la premi�re insertion � �t� supprim�e?
		//		// Risque de chercher une key a travers toute la hash_table qui a �t� supprim�e. BADDDDD!!!!!
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
		inline Value_Type* hash_table_get(const Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>& hash_table, Hash_Type hash, Key_Type& key)
		{
			ZoneScopedN("hash_table_get");

			fstd::core::Assert(hash_table.compare_function);

			while (true)
			{
				size_t bucket_index = hash / _bucket_size;
				size_t value_index = hash - (bucket_index * _bucket_size);
				auto* bucket = get_array_element(hash_table.buckets, bucket_index);

				if (bucket->nb_values == 0) // bucket is not iniatilized
					return nullptr;

				if (boolean_array_get(bucket->init_flags, value_index) == false)
					return nullptr;

				auto& value_pod = bucket->table[value_index];
				if (hash_table.compare_function(key, value_pod.key) == true)
					return &value_pod.value;
					
				// Increment the hash due to the collision, but we also take care of keeping it in the range.
				hash = (hash + 1) % std::numeric_limits<Hash_Type>::max();
			}
		}

		template<typename Hash_Type, typename Key_Type, typename Value_Type, size_t _bucket_size>
		inline size_t hash_table_get_size(const Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>& hash_table)
		{
			fstd::core::Assert(hash_table.compare_function);

			return hash_table.size;
		}

		template<typename Hash_Type, typename Key_Type, typename Value_Type, size_t _bucket_size>
		inline typename Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>::Iterator hash_table_begin(const Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>& hash_table)
		{
			ZoneScopedN("hash_table_begin");

			// We search the first valid value

			typename Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>::Iterator it;

			it.hash_table = &hash_table;

			for (it.bucket_index = 0; it.bucket_index < get_array_size(hash_table.buckets); it.bucket_index++)
			{
				auto* bucket = get_array_element(hash_table.buckets, it.bucket_index);

				size_t bucket_size = bucket->nb_values == 0 ? 0 : _bucket_size;
				for (it.slot_index = 0; it.slot_index < bucket_size; it.slot_index++) // @Warning some buckets aren't allocated so the size can be 0
				{
					if (boolean_array_get(bucket->init_flags, it.slot_index) == true)
						return it;
				}
			}

			return it;
		}

		template<typename Hash_Type, typename Key_Type, typename Value_Type, size_t _bucket_size>
		inline typename Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>::Iterator& hash_table_next(typename Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>::Iterator& it)
		{
			ZoneScopedN("hash_table_next");

			// We increment the slot index to start to test the next value and we keep incrementing until a value is found

			it.slot_index++;

			for (; it.bucket_index < get_array_size(it.hash_table->buckets); it.bucket_index++)
			{
				auto* bucket = get_array_element(it.hash_table->buckets, it.bucket_index);

				size_t bucket_size = bucket->nb_values == 0 ? 0 : _bucket_size;
				for (; it.slot_index < bucket_size; it.slot_index++) // @Warning some buckets aren't allocated so the size can be 0
				{
					if (boolean_array_get(bucket->init_flags, it.slot_index) == true)
						return it;
				}
				it.slot_index = 0;
			}

			return it;
		}

		template<typename Hash_Type, typename Key_Type, typename Value_Type, size_t _bucket_size>
		inline typename Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>::Iterator hash_table_end(const Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>& hash_table)
		{
			ZoneScopedN("hash_table_end");

			typename Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>::Iterator it;

			it.hash_table = &hash_table;

			it.bucket_index = get_array_size(hash_table.buckets);
			it.slot_index = 0; // Not _bucket_size because when we go after the last bucket the slot_index is reset to 0 (take a look at hash_table_next implementation)

			return it;
		}

		template<typename Hash_Type, typename Key_Type, typename Value_Type, size_t _bucket_size>
		inline bool equals(typename Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>::Iterator& a, typename Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>::Iterator& b)
		{
			ZoneScopedN("equals");

			return a.hash_table == b.hash_table && a.bucket_index == b.bucket_index && a.slot_index == b.slot_index;
		}

		template<typename Hash_Type, typename Key_Type, typename Value_Type, size_t _bucket_size>
		inline Value_Type* hash_table_get(typename Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>::Iterator& it)
		{
			ZoneScopedN("hash_table_get (iterator)");

			auto* bucket = get_array_element(it.hash_table->buckets, it.bucket_index);

			if (bucket == nullptr)
				return nullptr;

			if (boolean_array_get(bucket->init_flags, it.slot_index) == false)
				return nullptr;

			auto& value_pod = bucket->table[it.slot_index];
			return &value_pod.value;
		}

		template<typename Hash_Type, typename Key_Type, typename Value_Type, size_t _bucket_size>
		inline void log_stats(Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>& hash_table, fstd::core::Logger& logger)
		{
			ZoneScopedN("log_stats");

			size_t nb_used_buckets = 0;
			size_t bucket_size = _bucket_size * sizeof(Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>::Value_POD);
			size_t memory_size = sizeof(hash_table.buckets) +
				get_array_size(hash_table.buckets) * sizeof(Hash_Table<Hash_Type, Key_Type, Value_Type, _bucket_size>::Bucket);

			for (size_t bucket_index = 0; bucket_index < get_array_size(hash_table.buckets); bucket_index++)
			{
				auto* bucket = get_array_element(hash_table.buckets, bucket_index);

				if (bucket->nb_values > 0) {
					nb_used_buckets++;
					memory_size += bucket_size;
					// @TODO + init_flags size
				}
			}

			fstd::core::log(logger, fstd::core::Log_Level::info,
				"\n[Hash_Table]\n"
				"    Nb used buckets: %d\n"
				"    Nb total buckets: %d\n"
				"    Occupied memory per allocated bucket: %d bytes\n"
				"    Occupied memory by entire Hash_Table: %d bytes\n", nb_used_buckets, get_array_size(hash_table.buckets), bucket_size, memory_size);
		}

		// @TODO I should add a log of colisions like I had with the old Keyword_Hash_Table used by the lexer
	}
}

// Hash functions
// http://www.cse.yorku.ca/~oz/hash.html
// djb2 seems good, but is it good enough for me?
//
// New hash32 and hash64
// http://burtleburtle.net/bob/hash/evahash.html

// http://burtleburtle.net/bob/c/lookup8.c