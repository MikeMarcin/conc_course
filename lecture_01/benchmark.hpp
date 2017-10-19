#pragma once 
#include <cstdint>
#include <intrin.h>

inline uint64_t read_timestamp() {
    uint32_t aux;
    return __rdtscp( &aux );
}

namespace detail {
    void UseCharPointer( char const volatile* );
}

template <class Tp>
inline __forceinline void DoNotOptimize( Tp const& value ) {
    detail::UseCharPointer( &reinterpret_cast<char const volatile&>(value) );
    _ReadWriteBarrier();
}

inline __forceinline void ClobberMemory() {
    _ReadWriteBarrier();
}
