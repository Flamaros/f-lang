#pragma once

namespace fight-std {
    struct string_ref
    {
        uint8_t*    ref_ptr;
        uin32_t     length;
        uint8_t*    original_ptr;
        uint8_t*    original_length;
    };
}
