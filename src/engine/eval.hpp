#pragma once
#include "board.hpp"

namespace engine {
    class Evaluator {
    public:
        // Static evaluation in centipawns from the side to move's perspective
        static int evaluate(const Board& board);
        // Nominal material value, used for move ordering and pruning margins
        static int piece_value(Piece p);
    };
}
