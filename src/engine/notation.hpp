#pragma once
#include "board.hpp"
#include <string>
#include <vector>

namespace engine {
    std::string square_to_string(Square sq);
    std::string move_to_uci(const Move& move);
    Move move_from_uci(Board& board, const std::string& uci);
    std::string move_to_san(Board& board, const Move& move);
    GameResult position_result(Board& board);
    std::string result_string(GameResult r);

    std::string moves_to_pgn(const std::vector<std::string>& san_list, GameResult result,
                             const std::string& white_name = "White",
                             const std::string& black_name = "Black");
    std::vector<Move> pgn_to_moves(Board& board, const std::string& pgn_text);
}
