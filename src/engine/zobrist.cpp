#include "zobrist.hpp"
#include <random>

namespace engine {
    U64 Zobrist::piece_keys[6][2][64];
    U64 Zobrist::side_key;
    U64 Zobrist::castling_keys[16];
    U64 Zobrist::enpassant_keys[64];

    void Zobrist::init() {
        std::mt19937_64 rng(1070372ULL); // Fixed seed for reproducible hashes
        
        for (int p = 0; p < 6; p++) {
            for (int c = 0; c < 2; c++) {
                for (int sq = 0; sq < 64; sq++) {
                    piece_keys[p][c][sq] = rng();
                }
            }
        }
        
        side_key = rng();
        
        for (int i = 0; i < 16; i++) castling_keys[i] = rng();
        for (int i = 0; i < 64; i++) enpassant_keys[i] = rng();
    }
}
