#pragma once

#include <fstd/language/types.hpp>

template<typename T>
inline void set_flag(T& value, T flag) {
value = (T)((uint32_t)value | (uint32_t)flag);
}

template<typename T>
inline void unset_flag(T& value, T flag) {
    value = (T)((uint32_t)value & ~(uint32_t)flag);
}

template<typename T>
inline bool is_flag_set(T value, T flag) {
    return ((uint32_t)value & (uint32_t)flag) != 0;
}
