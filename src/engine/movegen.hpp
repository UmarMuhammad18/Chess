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

        static void gen_pawn_moves(const Board& board, std::vector<Move>& moves, Color us);
        static void gen_castling_moves(const Board& board, std::vector<Move>& moves, Color us);
    };
}
