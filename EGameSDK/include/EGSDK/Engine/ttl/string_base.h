#pragma once
#include <cstdint>

namespace ttl {
    template <typename T>
    class string_base;

    template <>
    class string_base<char> {
        char* m_Buffer;
        uint32_t m_Size;
        uint32_t m_Capacity;
    };
}