#include "attacks.hpp"
#include "utils.hpp"
#include <vector>

namespace engine::attacks {
    namespace {
        constexpr U64 FILE_A = 0x0101010101010101ULL;
        constexpr U64 FILE_H = 0x8080808080808080ULL;
        constexpr U64 RANK_1 = 0x00000000000000FFULL;
        constexpr U64 RANK_8 = 0xFF00000000000000ULL;

        struct Magic {
            U64  mask;
            U64  magic;
            U64* table;
            int  shift;

            unsigned index(U64 occ) const { return static_cast<unsigned>(((occ & mask) * magic) >> shift); }
        };

        U64 pawn_table[2][64];
        U64 knight_table[64];
        U64 king_table[64];

        Magic bishop_magics[64];
        Magic rook_magics[64];
        U64   bishop_attack_table[5248];   // sum of 2^bits over all squares
        U64   rook_attack_table[102400];

        bool initialised = false;

        U64 ray(int sq, U64 occ, int df, int dr) {
            U64 result = 0;
            int f = sq % 8 + df, r = sq / 8 + dr;
            while (f >= 0 && f < 8 && r >= 0 && r < 8) {
                U64 bit = 1ULL << (r * 8 + f);
                result |= bit;
                if (occ & bit) break;
                f += df; r += dr;
            }
            return result;
        }

        // Deterministic xorshift so magic generation is reproducible
        U64 rng_state = 0x9E3779B97F4A7C15ULL;
        U64 rand64() {
            rng_state ^= rng_state >> 12;
            rng_state ^= rng_state << 25;
            rng_state ^= rng_state >> 27;
            return rng_state * 2685821657736338717ULL;
        }
        U64 sparse_rand() { return rand64() & rand64() & rand64(); }

        void init_magics(Magic* magics, U64* table, U64 (*slow)(Square, U64)) {
            std::vector<U64> occupancies(4096), references(4096);
            std::vector<int> epoch(4096, 0);
            int attempt = 0;
            U64* next = table;

            for (int sq = 0; sq < 64; sq++) {
                Square s = static_cast<Square>(sq);
                U64 edges = ((RANK_1 | RANK_8) & ~(RANK_1 << (8 * (sq / 8))))
                          | ((FILE_A | FILE_H) & ~(FILE_A << (sq % 8)));

                Magic& m = magics[sq];
                m.mask  = slow(s, 0) & ~edges;
                m.shift = 64 - popcount(m.mask);
                m.table = next;

                // Enumerate every subset of the mask (Carry-Rippler)
                int size = 0;
                U64 b = 0;
                do {
                    occupancies[size] = b;
                    references[size]  = slow(s, b);
                    size++;
                    b = (b - m.mask) & m.mask;
                } while (b);
                next += size;

                // Find a multiplier that maps every subset without destructive collisions
                for (bool found = false; !found;) {
                    do { m.magic = sparse_rand(); } while (popcount((m.mask * m.magic) >> 56) < 6);
                    attempt++;
                    found = true;
                    for (int i = 0; i < size; i++) {
                        unsigned idx = m.index(occupancies[i]);
                        if (epoch[idx] < attempt) {
                            epoch[idx] = attempt;
                            m.table[idx] = references[i];
                        } else if (m.table[idx] != references[i]) {
                            found = false;
                            break;
                        }
                    }
                }
            }
        }
    }

    U64 bishop_slow(Square sq, U64 occ) {
        int s = static_cast<int>(sq);
        return ray(s, occ, 1, 1) | ray(s, occ, -1, 1) | ray(s, occ, 1, -1) | ray(s, occ, -1, -1);
    }

    U64 rook_slow(Square sq, U64 occ) {
        int s = static_cast<int>(sq);
        return ray(s, occ, 1, 0) | ray(s, occ, -1, 0) | ray(s, occ, 0, 1) | ray(s, occ, 0, -1);
    }

    void init() {
        if (initialised) return;

        for (int sq = 0; sq < 64; sq++) {
            U64 bb = 1ULL << sq;

            pawn_table[0][sq] = ((bb << 7) & ~FILE_H) | ((bb << 9) & ~FILE_A);
            pawn_table[1][sq] = ((bb >> 7) & ~FILE_A) | ((bb >> 9) & ~FILE_H);

            U64 n = 0;
            n |= (bb << 17) & ~FILE_A;
            n |= (bb << 15) & ~FILE_H;
            n |= (bb << 10) & ~(FILE_A | (FILE_A << 1));
            n |= (bb << 6)  & ~(FILE_H | (FILE_H >> 1));
            n |= (bb >> 17) & ~FILE_H;
            n |= (bb >> 15) & ~FILE_A;
            n |= (bb >> 10) & ~(FILE_H | (FILE_H >> 1));
            n |= (bb >> 6)  & ~(FILE_A | (FILE_A << 1));
            knight_table[sq] = n;

            U64 k = ((bb << 1) & ~FILE_A) | ((bb >> 1) & ~FILE_H);
            k |= (bb << 8) | (bb >> 8);
            k |= ((bb << 9) & ~FILE_A) | ((bb >> 9) & ~FILE_H);
            k |= ((bb << 7) & ~FILE_H) | ((bb >> 7) & ~FILE_A);
            king_table[sq] = k;
        }

        init_magics(bishop_magics, bishop_attack_table, bishop_slow);
        init_magics(rook_magics, rook_attack_table, rook_slow);
        initialised = true;
    }

    U64 pawn(Color c, Square sq) { return pawn_table[static_cast<int>(c)][static_cast<int>(sq)]; }
    U64 knight(Square sq)        { return knight_table[static_cast<int>(sq)]; }
    U64 king(Square sq)          { return king_table[static_cast<int>(sq)]; }

    U64 bishop(Square sq, U64 occ) {
        const Magic& m = bishop_magics[static_cast<int>(sq)];
        return m.table[m.index(occ)];
    }

    U64 rook(Square sq, U64 occ) {
        const Magic& m = rook_magics[static_cast<int>(sq)];
        return m.table[m.index(occ)];
    }
}
