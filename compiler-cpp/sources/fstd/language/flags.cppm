export module fstd.language.flags;

import fstd.language.types;

export
template<typename T>
inline void set_flag(T& value, T flag) {
    value = (T)((uint32_t)value | (uint32_t)flag);
}

export
template<typename T>
inline void unset_flag(T& value, T flag) {
    value = (T)((uint32_t)value & ~(uint32_t)flag);
}

export
template<typename T>
inline bool is_flag_set(T value, T flag) {
    return ((uint32_t)value & (uint32_t)flag) != 0;
}
