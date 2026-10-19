#pragma once
#include "board.hpp"

namespace engine {
    class MoveGen {
    public:
        // Initialises attack tables (idempotent)
        static void init();

        static bool is_square_attacked(const Board& board, Square sq, Color by_color);
        // True if the king of color c is currently attacked
        static bool is_king_attacked(const Board& board, Color c);
        // True if the side to move is in check
        static bool in_check(const Board& board) { return is_king_attacked(board, board.get_side_to_move()); }

        static void generate_pseudo_legal_moves(const Board& board, MoveList& moves);
        // Captures, en passant and queen promotions only (for quiescence search)
        static void generate_captures(const Board& board, MoveList& moves);
        static MoveList generate_legal_moves(Board& board);

    private:
        template <bool CapturesOnly>
        static void generate(const Board& board, MoveList& moves);
        template <bool CapturesOnly>
        static void gen_pawn_moves(const Board& board, MoveList& moves, Color us);
        static void gen_castling_moves(const Board& board, MoveList& moves, Color us);
    };
}
