#pragma once
#include "board.hpp"
#include "movegen.hpp"
#include "transposition.hpp"

namespace engine {
    class Search {
    public:
        Search() : tt(16) {} // 16MB TT
        Move search_best_move(Board& board, int depth);
        TranspositionTable tt;
    private:
        int negamax(Board& board, int depth, int alpha, int beta);
        int score_move(const Board& board, const Move& move, Move tt_move);
    };
}
