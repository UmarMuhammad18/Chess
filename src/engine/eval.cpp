#include "eval.hpp"
#include "utils.hpp"

namespace engine {
    // Piece-square tables, white's perspective, a1 = index 0 (rank 1 first).
    // Black indexes with sq ^ 56.
    static const int PST_PAWN[64] = {
         0,  0,  0,  0,  0,  0,  0,  0,
         5, 10, 10,-20,-20, 10, 10,  5,
         5, -5,-10,  0,  0,-10, -5,  5,
         0,  0,  0, 20, 20,  0,  0,  0,
         5,  5, 10, 25, 25, 10,  5,  5,
        10, 10, 20, 30, 30, 20, 10, 10,
        50, 50, 50, 50, 50, 50, 50, 50,
         0,  0,  0,  0,  0,  0,  0,  0
    };
    static const int PST_KNIGHT[64] = {
        -50,-40,-30,-30,-30,-30,-40,-50,
        -40,-20,  0,  5,  5,  0,-20,-40,
        -30,  5, 10, 15, 15, 10,  5,-30,
        -30,  0, 15, 20, 20, 15,  0,-30,
        -30,  5, 15, 20, 20, 15,  5,-30,
        -30,  0, 10, 15, 15, 10,  0,-30,
        -40,-20,  0,  0,  0,  0,-20,-40,
        -50,-40,-30,-30,-30,-30,-40,-50
    };
    static const int PST_BISHOP[64] = {
        -20,-10,-10,-10,-10,-10,-10,-20,
        -10,  5,  0,  0,  0,  0,  5,-10,
        -10, 10, 10, 10, 10, 10, 10,-10,
        -10,  0, 10, 10, 10, 10,  0,-10,
        -10,  5,  5, 10, 10,  5,  5,-10,
        -10,  0,  5, 10, 10,  5,  0,-10,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -20,-10,-10,-10,-10,-10,-10,-20
    };
    static const int PST_ROOK[64] = {
         0,  0,  0,  5,  5,  0,  0,  0,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
         5, 10, 10, 10, 10, 10, 10,  5,
         0,  0,  0,  0,  0,  0,  0,  0
    };
    static const int PST_QUEEN[64] = {
        -20,-10,-10, -5, -5,-10,-10,-20,
        -10,  0,  5,  0,  0,  0,  0,-10,
        -10,  5,  5,  5,  5,  5,  0,-10,
          0,  0,  5,  5,  5,  5,  0, -5,
         -5,  0,  5,  5,  5,  5,  0, -5,
        -10,  0,  5,  5,  5,  5,  0,-10,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -20,-10,-10, -5, -5,-10,-10,-20
    };
    static const int PST_KING_MG[64] = {
         20, 30, 10,  0,  0, 10, 30, 20,
         20, 20,  0,  0,  0,  0, 20, 20,
        -10,-20,-20,-20,-20,-20,-20,-10,
        -20,-30,-30,-40,-40,-30,-30,-20,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30
    };
    static const int PST_KING_EG[64] = {
        -50,-30,-30,-30,-30,-30,-30,-50,
        -30,-30,  0,  0,  0,  0,-30,-30,
        -30,-10, 20, 30, 30, 20,-10,-30,
        -30,-10, 30, 40, 40, 30,-10,-30,
        -30,-10, 30, 40, 40, 30,-10,-30,
        -30,-10, 20, 30, 30, 20,-10,-30,
        -30,-20,-10,  0,  0,-10,-20,-30,
        -50,-40,-30,-20,-20,-30,-40,-50
    };

    static const int* PST[6] = {
        PST_PAWN, PST_KNIGHT, PST_BISHOP, PST_ROOK, PST_QUEEN, PST_KING_MG
    };

    static int pst_value(Piece p, Color c, int sq, bool endgame) {
        int idx = (c == Color::WHITE) ? sq : (sq ^ 56);
        if (p == Piece::KING) return endgame ? PST_KING_EG[idx] : PST_KING_MG[idx];
        return PST[static_cast<int>(p)][idx];
    }

    int Evaluator::evaluate(const Board& board) {
        int values[] = {100, 320, 330, 500, 900, 0};
        int score = 0;
        int phase_mat = 0;

        for (int i = 0; i < 5; i++) {
            phase_mat += popcount(board.get_pieces(static_cast<Piece>(i), Color::WHITE)) * values[i];
            phase_mat += popcount(board.get_pieces(static_cast<Piece>(i), Color::BLACK)) * values[i];
        }
        bool endgame = phase_mat < 1600;

        for (int i = 0; i < 6; i++) {
            Piece p = static_cast<Piece>(i);
            U64 ww = board.get_pieces(p, Color::WHITE);
            while (ww) {
                int sq = static_cast<int>(pop_lsb(ww));
                score += values[i] + pst_value(p, Color::WHITE, sq, endgame);
            }
            U64 bb = board.get_pieces(p, Color::BLACK);
            while (bb) {
                int sq = static_cast<int>(pop_lsb(bb));
                score -= values[i] + pst_value(p, Color::BLACK, sq, endgame);
            }
        }

        return (board.get_side_to_move() == Color::WHITE) ? score : -score;
    }
}
