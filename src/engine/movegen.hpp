#pragma once
#include "board.hpp"
#include <vector>

namespace engine {
    class MoveGen {
    public:
        static void init();
        static bool is_square_attacked(const Board& board, Square sq, Color by_color);
        static bool in_check(const Board& board);
        static std::vector<Move> generate_pseudo_legal_moves(const Board& board);
        static std::vector<Move> generate_legal_moves(Board& board);
        static std::vector<Move> generate_legal_noisy(Board& board);
    private:
        static U64 knight_attacks[64];
        static U64 king_attacks[64];
        static U64 pawn_attacks[2][64];

        static U64 get_ray_attacks(Square sq, U64 occupancy, int dir_index);
        static U64 rook_attacks_otf(Square sq, U64 occ);
        static U64 bishop_attacks_otf(Square sq, U64 occ);

        static U64 rook_magic[64];
        static U64 bishop_magic[64];
        static int  rook_shift[64];
        static int  bishop_shift[64];
        static U64  rook_mask[64];
        static U64  bishop_mask[64];
        static U64* rook_attacks_table[64];
        static U64* bishop_attacks_table[64];
        static std::vector<U64> rook_table_storage;
        static std::vector<U64> bishop_table_storage;

        static U64 rook_attacks(Square sq, U64 occ);
        static U64 bishop_attacks(Square sq, U64 occ);
        static void init_magics();

        static void gen_pawn_moves(const Board& board, std::vector<Move>& moves, Color us);
        static void gen_castling_moves(const Board& board, std::vector<Move>& moves, Color us);
    };
}
