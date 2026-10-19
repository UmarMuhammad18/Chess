#pragma once
#include "board.hpp"
#include <string>

namespace engine {
    std::string square_name(Square sq);
    // Long algebraic notation used by UCI, e.g. "e2e4", "e7e8q"
    std::string move_to_uci(Move m);
    // Returns the matching legal move, or a null move if the string is not legal here
    Move parse_uci_move(Board& board, const std::string& text);
    // Standard algebraic notation for a legal move in the given position, e.g. "Nxf3+", "O-O", "e8=Q#"
    std::string move_to_san(Board& board, Move m);
}
