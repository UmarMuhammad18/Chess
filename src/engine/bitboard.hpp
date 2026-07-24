#pragma once
#include "types.hpp"

namespace engine {
    inline void set_bit(U64& bb, Square sq) {
        bb |= (1ULL << static_cast<int>(sq));
    }
    
    inline void clear_bit(U64& bb, Square sq) {
        bb &= ~(1ULL << static_cast<int>(sq));
    }
    
    inline bool get_bit(U64 bb, Square sq) {
        return (bb & (1ULL << static_cast<int>(sq))) != 0;
    }
}
