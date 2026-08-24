#include "notation.hpp"
#include "movegen.hpp"
#include "utils.hpp"

namespace engine {
    std::string square_to_string(Square sq) {
        int s = static_cast<int>(sq);
        if (s < 0 || s > 63) return "-";
        std::string out;
        out += static_cast<char>('a' + (s % 8));
        out += static_cast<char>('1' + (s / 8));
        return out;
    }

    std::string move_to_uci(const Move& move) {
        if (move.data == 0) return "0000";
        std::string s = square_to_string(move.get_from()) + square_to_string(move.get_to());
        if (move.is_promotion()) {
            Piece p = move.promo_piece();
            if (p == Piece::QUEEN) s += 'q';
            else if (p == Piece::ROOK) s += 'r';
            else if (p == Piece::BISHOP) s += 'b';
            else s += 'n';
        }
        return s;
    }

    Move move_from_uci(Board& board, const std::string& uci) {
        if (uci.size() < 4) return Move{};
        auto legal = MoveGen::generate_legal_moves(board);
        for (const auto& m : legal) {
            if (move_to_uci(m) == uci) return m;
        }
        return Move{};
    }

    static char piece_letter(Piece p) {
        switch (p) {
            case Piece::KNIGHT: return 'N';
            case Piece::BISHOP: return 'B';
            case Piece::ROOK:   return 'R';
            case Piece::QUEEN:  return 'Q';
            case Piece::KING:   return 'K';
            default: return 0;
        }
    }

    static std::string check_suffix(Board& board, const Move& move) {
        board.make_move(move);
        GameResult r = position_result(board);
        bool chk = MoveGen::in_check(board);
        board.unmake_move(move);
        if (r == GameResult::WhiteMates || r == GameResult::BlackMates) return "#";
        if (chk) return "+";
        return "";
    }

    std::string move_to_san(Board& board, const Move& move) {
        MoveFlag flag = move.get_flag();
        if (flag == MoveFlag::KingCastle) return "O-O" + check_suffix(board, move);
        if (flag == MoveFlag::QueenCastle) return "O-O-O" + check_suffix(board, move);

        Square from = move.get_from();
        Square to = move.get_to();
        Piece moving = board.piece_on(from);
        bool capture = move.is_capture();

        std::string s;
        if (moving == Piece::PAWN) {
            if (capture) {
                s += static_cast<char>('a' + (static_cast<int>(from) % 8));
                s += 'x';
            }
            s += square_to_string(to);
            if (move.is_promotion()) {
                s += '=';
                s += piece_letter(move.promo_piece());
            }
        } else {
            s += piece_letter(moving);
            auto legal = MoveGen::generate_legal_moves(board);
            bool file_clash = false, rank_clash = false, any = false;
            int from_f = static_cast<int>(from) % 8;
            int from_r = static_cast<int>(from) / 8;
            for (const auto& m : legal) {
                if (m.data == move.data) continue;
                if (m.get_to() != to) continue;
                if (board.piece_on(m.get_from()) != moving) continue;
                any = true;
                int f = static_cast<int>(m.get_from()) % 8;
                int r = static_cast<int>(m.get_from()) / 8;
                if (f == from_f) file_clash = true;
                if (r == from_r) rank_clash = true;
            }
            if (any) {
                if (!file_clash) s += static_cast<char>('a' + from_f);
                else if (!rank_clash) s += static_cast<char>('1' + from_r);
                else {
                    s += static_cast<char>('a' + from_f);
                    s += static_cast<char>('1' + from_r);
                }
            }
            if (capture) s += 'x';
            s += square_to_string(to);
        }

        return s + check_suffix(board, move);
    }

    GameResult position_result(Board& board) {
        if (board.is_fifty_move_draw()) return GameResult::DrawFifty;
        if (board.repetition_count() >= 3) return GameResult::DrawRepetition;
        if (board.is_insufficient_material()) return GameResult::DrawMaterial;

        auto moves = MoveGen::generate_legal_moves(board);
        if (!moves.empty()) return GameResult::Ongoing;

        if (MoveGen::in_check(board)) {
            return board.get_side_to_move() == Color::WHITE ? GameResult::BlackMates : GameResult::WhiteMates;
        }
        return GameResult::Stalemate;
    }

    std::string result_string(GameResult r) {
        switch (r) {
            case GameResult::Ongoing:        return "Playing";
            case GameResult::WhiteMates:     return "Checkmate - White wins";
            case GameResult::BlackMates:     return "Checkmate - Black wins";
            case GameResult::Stalemate:      return "Draw - stalemate";
            case GameResult::DrawFifty:      return "Draw - 50-move rule";
            case GameResult::DrawRepetition: return "Draw - threefold";
            case GameResult::DrawMaterial:   return "Draw - insufficient material";
            case GameResult::WhiteResigns:   return "Black wins - White resigned";
            case GameResult::BlackResigns:   return "White wins - Black resigned";
        }
        return "";
    }
}
