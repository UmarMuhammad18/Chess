#pragma once
#include "movegen.hpp"
#include <cstdint>

namespace engine {
    // Counts leaf nodes of the legal move tree; the standard move generator correctness check
    inline uint64_t perft(Board& board, int depth) {
        MoveList moves = MoveGen::generate_legal_moves(board);
        if (depth <= 1) return depth <= 0 ? 1 : static_cast<uint64_t>(moves.size());
        uint64_t nodes = 0;
        for (const Move& m : moves) {
            board.make_move(m);
            nodes += perft(board, depth - 1);
            board.unmake_move(m);
        }
        return nodes;
    }
}
