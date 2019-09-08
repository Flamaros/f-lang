#pragma once

#include <vector>

#include <assert.h>

namespace utilities
{
    template<typename T>
    void    remove_fast(std::vector<T>& array, size_t index) {
        assert(index < array.size());
        if (index >= array.size())
            return;

        if (index < array.size() - 1)
            array[index] = array.back();
        array.resize(array.size() - 1);
    }

    template<typename T>
    size_t  search_fast(std::vector<T>& array, T value) {
        // TODO distribuate over core of CPU if the array is large enough
        // Use a thread pool
        for (size_t i = 0; i < array.size(); i++) {
            if (array[i] == value) {
                return i;
            }
        }
        return (size_t)-1;
    }
}
