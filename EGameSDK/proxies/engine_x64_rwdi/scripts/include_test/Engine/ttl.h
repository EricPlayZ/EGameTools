#pragma once
#include <cstdint>

namespace ttl {
    namespace vector_allocators {
        template <typename T>
        class heap_allocator;
    }

    template <typename T>
    class string_base;

    template <>
    class string_base<char> {
        char* m_Buffer;
        uint32_t m_Size;
        uint32_t m_Capacity;
    };

    template <typename T>
    class string_const;

    template <typename T1, typename T2, size_t T3>
    class vector;

    template <typename T1, typename T2, typename T3, typename T4>
    class map;

    template <typename T1>
    struct less;

    class allocator;

    template <typename T1, typename T2>
    class list {
    public:
        class const_reverse_iterator;
    };
}