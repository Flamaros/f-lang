#pragma once

#include <fstd/language/types.hpp>

#include <fstd/system/allocator.hpp>

// @Warning This array isn't resizable!

namespace fstd
{
	namespace container
	{
		template<size_t nb_values>
		struct Boolean_Array
		{
			static inline bool is_bit_set(const uint32_t& var, size_t pos)	{ return (var >> pos) & 0b1; }
			static inline void set_bit(uint32_t& var, size_t pos)			{ var |= 1UL << pos; }
			static inline void unset_bit(uint32_t& var, size_t pos)			{ var &= ~(1UL << pos); }

			// Compile time variables
			using Chunk_Type = uint32_t;
			constexpr size_t nb_bits_per_chunk() { return sizeof(Chunk_Type) * 8; }
			constexpr size_t nb_chunks() { return nb_values / nb_bits_per_chunk(); }

			Chunk_Type* data;
		};

		template<size_t nb_values>
		inline void init(Boolean_Array<nb_values>& boolean_array)
		{
			boolean_array.data = nullptr;
		}

		template<size_t nb_values>
		inline void allocate(Boolean_Array<nb_values>& boolean_array)
		{
			ZoneScopedN("allocate");

			core::Assert(boolean_array.data == nullptr);

			boolean_array.data = (uint32_t*)system::allocate(boolean_array.nb_chunks() * sizeof(uint32_t));
			system::zero_memory(boolean_array.data, boolean_array.nb_chunks() * sizeof(uint32_t));
		}

		template<size_t nb_values>
		inline void release(Boolean_Array<nb_values>& boolean_array)
		{
			ZoneScopedN("release");

			system::free(boolean_array.data);
		}

		template<size_t nb_values>
		inline bool boolean_array_get(Boolean_Array<nb_values>& boolean_array, size_t bit_index)
		{
			ZoneScopedN("boolean_array_get");

			core::Assert(boolean_array.data);
			core::Assert(bit_index < nb_values);

			size_t chunk_index = bit_index / boolean_array.nb_bits_per_chunk();
			size_t bit_subindex = bit_index % boolean_array.nb_bits_per_chunk();

			return Boolean_Array<nb_values>::is_bit_set(boolean_array.data[chunk_index], bit_subindex);
		}

		template<size_t nb_values>
		inline void boolean_array_set(Boolean_Array<nb_values>& boolean_array, size_t bit_index, bool value)
		{
			ZoneScopedN("boolean_array_set");

			core::Assert(boolean_array.data);
			core::Assert(bit_index < nb_values);

			size_t chunk_index = bit_index / boolean_array.nb_bits_per_chunk();
			size_t bit_subindex = bit_index % boolean_array.nb_bits_per_chunk();

			// @SpeedUp can we do it in a branchless way?
			if (value)
				Boolean_Array<nb_values>::set_bit(boolean_array.data[chunk_index], bit_subindex);
			else
				Boolean_Array<nb_values>::unset_bit(boolean_array.data[chunk_index], bit_subindex);
		}

		// @TODO add searching methods for first or last set bit 
		// counting bits,...
	}
}
