export module fstd.language.string_view;

import fstd.language.string;

import fstd.language.types;

// string_view are also used for string literals as the memory of a literal is in the binary.
// In f-lang string literals will be serialized in the binary with the exact same struct than string_view.
// size will be stored at compile time.
// 
// Flamaros - 16 february 2020

namespace fstd {
	namespace language {
		export
		struct string_view
		{
			uint8_t*	ptr = nullptr;
			size_t		size = 0;

			// @TODO @CleanUp
			// I don't really want to put the operator here (directly in the struct)
			inline uint8_t& operator[](size_t index)
			{
				return ptr[index];
			}

			// @TODO @WTF??? Why I need a const version???
			inline const uint8_t& operator[](size_t index) const
			{
				return ptr[index];
			}
		};

		export inline void assign(string_view& str, uint8_t* string)
		{
			str.size = string_literal_size(string);
			str.ptr = string;
		}

		export inline void assign(string_view& str, uint8_t* start, size_t size)
		{
			str.ptr = start;
			str.size = size;
		}

		export inline void assign(string_view& str, const string& string)
		{
			str.ptr = to_utf8(string);
			str.size = get_string_size(string);
		}

		export inline void resize(string_view& str, size_t size)
		{
			str.size = size;
		}

		// @TODO should be in string, but C++ (cycle inclusion)
		export inline void copy(string& str, size_t position, const string_view& string)
		{
			memory::array_copy(str.buffer, position, string.ptr, string.size);
		}

		export inline size_t get_string_size(const string_view& str)
		{
			return str.size;
		}

		// @Warning be careful when using the resulting buffer
		// there is no ending '/0'
		export inline uint8_t* to_utf8(const string_view& str)
		{
			return str.ptr;
		}

		export inline bool are_equals(const string_view& a, const string_view& b)
		{
			if (a.size != b.size) {
				return false;
			}
			else if (a.ptr == b.ptr) {	// string_view on exact same string
				return true;
			}
			else {
				return system::memory_compare(a.ptr, b.ptr, a.size);
			}
		}

		// =====================================================================

		export
		struct UTF16LE_string_view
		{
			uint16_t*	ptr = nullptr;
			size_t		size = 0;

			// @TODO @CleanUp
			// I don't really want to put the operator here (directly in the struct)
			inline uint16_t& operator[](size_t index)
			{
				return ptr[index];
			}

			// @TODO @WTF??? Why I need a const version???
			inline const uint16_t& operator[](size_t index) const
			{
				return ptr[index];
			}
		};

		export inline void assign(UTF16LE_string_view& str, uint16_t* string)
		{
			str.size = string_literal_size(string);
			str.ptr = string;
		}

		export inline void assign(UTF16LE_string_view& str, uint16_t* start, size_t size)
		{
			str.ptr = start;
			str.size = size;
		}

		export inline void assign(UTF16LE_string_view& str, const UTF16LE_string& string)
		{
			str.ptr = to_utf16(string);
			str.size = get_string_size(string);
		}

		export inline void resize(UTF16LE_string_view& str, size_t size)
		{
			str.size = size;
		}

		// @TODO should be in string, but C++ (cycle inclusion)
		export inline void copy(UTF16LE_string& str, size_t position, const UTF16LE_string_view& string)
		{
			memory::array_copy(str.buffer, position, string.ptr, string.size);
		}

		export inline size_t get_string_size(const UTF16LE_string_view& str)
		{
			return str.size;
		}

		// @Warning be careful when using the resulting buffer
		// there is no ending '/0'
		export inline uint16_t* to_utf16(const UTF16LE_string_view& str)
		{
			return str.ptr;
		}

		export inline bool are_equals(const UTF16LE_string_view& a, const UTF16LE_string_view& b)
		{
			if (a.size != b.size) {
				return false;
			}
			else if (a.ptr == b.ptr) {	// string_view on exact same string
				return true;
			}
			else {
				return system::memory_compare(a.ptr, b.ptr, a.size);
			}
		}
	}
}
