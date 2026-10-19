#include "notation.hpp"
#include "movegen.hpp"

namespace engine {
    std::string square_name(Square sq) {
        int s = static_cast<int>(sq);
        return {static_cast<char>('a' + s % 8), static_cast<char>('1' + s / 8)};
    }

    std::string move_to_uci(Move m) {
        if (m.is_null()) return "0000";
        std::string s = square_name(m.get_from()) + square_name(m.get_to());
        switch (m.promo_piece()) {
            case Piece::KNIGHT: s += 'n'; break;
            case Piece::BISHOP: s += 'b'; break;
            case Piece::ROOK:   s += 'r'; break;
            case Piece::QUEEN:  s += 'q'; break;
            default: break;
        }
        return s;
    }

    Move parse_uci_move(Board& board, const std::string& text) {
        for (const Move& m : MoveGen::generate_legal_moves(board)) {
            if (move_to_uci(m) == text) return m;
        }
        return Move{};
    }

    std::string move_to_san(Board& board, Move m) {
        std::string san;
        if (m.get_flag() == MoveFlag::KingCastle) {
            san = "O-O";
        } else if (m.get_flag() == MoveFlag::QueenCastle) {
            san = "O-O-O";
        } else {
            Square from = m.get_from(), to = m.get_to();
            Piece p = board.piece_on(from);
            if (p == Piece::PAWN) {
                if (m.is_capture()) san += static_cast<char>('a' + static_cast<int>(from) % 8);
            } else {
                san += "PNBRQK"[static_cast<int>(p)];
                // Disambiguate between identical pieces that can reach the same square
                bool clash = false, same_file = false, same_rank = false;
                for (const Move& o : MoveGen::generate_legal_moves(board)) {
                    if (o.get_to() != to || o.get_from() == from || board.piece_on(o.get_from()) != p) continue;
                    clash = true;
                    if (static_cast<int>(o.get_from()) % 8 == static_cast<int>(from) % 8) same_file = true;
                    if (static_cast<int>(o.get_from()) / 8 == static_cast<int>(from) / 8) same_rank = true;
                }
                if (clash) {
                    std::string fs = square_name(from);
                    if (!same_file)      san += fs[0];
                    else if (!same_rank) san += fs[1];
                    else                 san += fs;
                }
            }
            if (m.is_capture()) san += 'x';
            san += square_name(to);
            if (m.is_promotion()) {
                san += '=';
                san += "PNBRQK"[static_cast<int>(m.promo_piece())];
            }
        }

        board.make_move(m);
        if (MoveGen::in_check(board)) {
            san += MoveGen::generate_legal_moves(board).empty() ? '#' : '+';
        }
        board.unmake_move(m);
        return san;
    }
}
