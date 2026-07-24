#pragma once
#include "types.hpp"

namespace engine {
    class Zobrist {
    public:
        static void init();
        static U64 piece_keys[6][2][64];
        static U64 side_key;
        static U64 castling_keys[16];
        static U64 enpassant_keys[64];
    };
}
