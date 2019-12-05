#include "path.hpp"

#include <fstd/system/allocator.hpp>

#include <iostream>

namespace fstd
{
	namespace system
	{
		void from_native(Path* path, const std::wstring& native_path)
		{
			reset_path(path);
			path->length = native_path.length() + 4 + 1;	// \0 doesn't count
			path->buffer = allocate(path->length * sizeof(wchar_t));

			memory_copy(path->buffer, (void*)LR"(\\?\)", 4 * sizeof(wchar_t));
			memory_copy(&((wchar_t*)path->buffer)[4], (void*)native_path.c_str(), native_path.length() * sizeof(wchar_t));
			((wchar_t*)path->buffer)[4 + native_path.length()] = 0;
		}

		wchar_t* to_native(Path* const path)
		{
			if (path->is_absolute) {
				return (wchar_t*)path->buffer;
			}
			else {
				return &((wchar_t*)path->buffer)[4];
			}
		}

		void reset_path(Path* path)
		{
			if (path->buffer != nullptr) {
				free(path->buffer);
				path->buffer = nullptr;
				path->length = 0;
			}
		}
	}
}
