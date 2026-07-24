#pragma once
#include "board.hpp"

namespace engine {
    class Evaluator {
    public:
        static int evaluate(const Board& board);
    };
}
