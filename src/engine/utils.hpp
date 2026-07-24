#pragma once
#include "types.hpp"
#include <bit>

namespace engine {
    inline int popcount(U64 bb) {
        return std::popcount(bb);
    }
    
    inline int lsb(U64 bb) {
        return std::countr_zero(bb);
    }
    
    inline U64 pop_lsb(U64& bb) {
        int sq = lsb(bb);
        bb &= bb - 1;
        return sq;
    }
}
