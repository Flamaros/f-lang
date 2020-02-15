#pragma once

#include <fstd/core/assert.hpp>

// Keyword_Hash_Table is a hash table that use buckets to save memory.
// The hash of the key is used directly as an index.
// We store key with value to be able to handle collisions on hashes.
// If a collision is detected we try to use the following index until we find one free.
//
//
// Flamaros - 15 february 2020


template<typename Hash_Type, typename Key_Type, typename Value_Type, Key_Type invalid_key, Value_Type default_value = Value_Type()>
class Keyword_Hash_Table
{
    static const size_t max_nb_values = (Hash_Type)0xffffffff'ffffffff + 1;
    static const size_t bucket_size = 32;
    static const size_t nb_buckets = max_nb_values / bucket_size;

    struct Full_Key_Value
    {
        Key_Type    key;
        Value_Type  value;
    };

public:
    Keyword_Hash_Table()
    {
        for (size_t i = 0; i < nb_buckets; i++)
            m_buckets[i] = nullptr;
    }

    void clear()
    {
        for (size_t i = 0; i < nb_buckets; i++) {
            delete m_buckets[i];
            m_buckets[i] = nullptr;
        }
        m_nb_used_buckets = 0;
        m_nb_collisions = 0;
    }

    void insert(const Hash_Type& hash, const Key_Type& key, const Value_Type& value)
    {
        size_t  bucket_index = hash / bucket_size;
        size_t  i = hash % bucket_size;

        for (size_t j = bucket_index; j < nb_buckets; j++) {
            if (m_buckets[j] == nullptr) {
                initialize_bucket(j);

                m_buckets[j][i].key = key;
                m_buckets[j][i].value = value;
                return;
            }
            else {  // In this case we may have a collision of hash
                for (; i < bucket_size; i++) {
                    if (m_buckets[j][i].key == invalid_key) {
                        break;
                    }
                    m_nb_collisions++;
                }

                if (m_buckets[j][i].key == invalid_key) {
                    m_buckets[j][i].key = key;
                    m_buckets[j][i].value = value;
                    return;
                }
            }
            i = 0;    // We'll go in the next bucket so we'll scan it from start
        }
    }

    Value_Type  find(const Hash_Type& hash, const Key_Type& key)
    {
        size_t  bucket_index = hash / bucket_size;

        if (m_buckets[bucket_index] == nullptr) {
            return default_value;
        }

        size_t  in_bucket_index = hash % bucket_size;

        for (size_t j = bucket_index; j < nb_buckets; j++) {
            for (size_t i = in_bucket_index; i < bucket_size; i++) {
                if (m_buckets[j][i].key == invalid_key) {
                    return default_value;
                }
                else if (m_buckets[j][i].key == key) {
                    return m_buckets[j][i].value;
                }
            }
            in_bucket_index = 0;    // We'll go in the next bucket so we'll scan it from start
        }

        return default_value;
    }

    size_t nb_used_buckets() const {
        return m_nb_used_buckets;
    }

    size_t nb_collisions() const {
        return m_nb_collisions;
    }

private:
    void initialize_bucket(size_t bucket_index)
    {
        // Alloc
        // Initialize with default key and value

        fstd::core::Assert(m_buckets[bucket_index] == nullptr);

        m_buckets[bucket_index] = new Full_Key_Value[bucket_size];
        for (size_t i = 0; i < bucket_size; i++) {
            m_buckets[bucket_index][i].key = invalid_key;
        }

        m_nb_used_buckets++;
    }

    Full_Key_Value* m_buckets[nb_buckets];
    size_t          m_nb_used_buckets = 0;
    size_t          m_nb_collisions = 0;
};
