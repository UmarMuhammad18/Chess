#include "eval.hpp"
#include "utils.hpp"

namespace engine {
    int Evaluator::evaluate(const Board& board) {
        int score = 0;
        int values[] = {100, 300, 300, 500, 900, 20000}; // P, N, B, R, Q, K
        
        for (int i = 0; i < 6; i++) {
            Piece p = static_cast<Piece>(i);
            score += popcount(board.get_pieces(p, Color::WHITE)) * values[i];
            score -= popcount(board.get_pieces(p, Color::BLACK)) * values[i];
        }
        
        // Return score from perspective of side to move
        return (board.get_side_to_move() == Color::WHITE) ? score : -score;
    }
}
