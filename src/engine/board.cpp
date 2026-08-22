#include "board.hpp"
#include "zobrist.hpp"
#include <cctype>
#include <iostream>
#include <sstream>

namespace engine {
    Board::Board() { init(); }

    void Board::init() {
        piece_bbs.fill(0);
        color_bbs.fill(0);
        side_to_move = Color::WHITE;
        en_passant_sq = Square::NONE;
        castling_rights = 0;
        half_move_clock = 0;
        full_move_number = 1;
        hash_key = 0;
        history.clear();
    }

    void Board::set_piece_at(Piece p, Color c, Square sq) {
        set_bit(piece_bbs[static_cast<int>(p)], sq);
        set_bit(color_bbs[static_cast<int>(c)], sq);
        hash_key ^= Zobrist::piece_keys[static_cast<int>(p)][static_cast<int>(c)][static_cast<int>(sq)];
    }

    void Board::remove_piece_at(Piece p, Color c, Square sq) {
        clear_bit(piece_bbs[static_cast<int>(p)], sq);
        clear_bit(color_bbs[static_cast<int>(c)], sq);
        hash_key ^= Zobrist::piece_keys[static_cast<int>(p)][static_cast<int>(c)][static_cast<int>(sq)];
    }

    Piece Board::piece_on(Square sq) const {
        for (int i = 0; i < 6; i++)
            if (get_bit(piece_bbs[i], sq)) return static_cast<Piece>(i);
        return Piece::NONE;
    }

    Color Board::color_on(Square sq) const {
        if (get_bit(color_bbs[0], sq)) return Color::WHITE;
        if (get_bit(color_bbs[1], sq)) return Color::BLACK;
        return Color::BOTH;
    }

    bool Board::load_fen(const std::string& fen) {
        init();
        std::istringstream iss(fen);
        std::string board_part, active_color, castling, en_passant, half_move, full_move;
        iss >> board_part >> active_color >> castling >> en_passant >> half_move >> full_move;

        int rank = 7, file = 0;
        for (char c : board_part) {
            if (c == '/') { rank--; file = 0; }
            else if (isdigit(static_cast<unsigned char>(c))) { file += (c - '0'); }
            else {
                Square sq = static_cast<Square>(rank * 8 + file);
                Color col = isupper(static_cast<unsigned char>(c)) ? Color::WHITE : Color::BLACK;
                Piece p;
                switch (tolower(static_cast<unsigned char>(c))) {
                    case 'p': p = Piece::PAWN; break;
                    case 'n': p = Piece::KNIGHT; break;
                    case 'b': p = Piece::BISHOP; break;
                    case 'r': p = Piece::ROOK; break;
                    case 'q': p = Piece::QUEEN; break;
                    case 'k': p = Piece::KING; break;
                    default: return false;
                }
                set_piece_at(p, col, sq);
                file++;
            }
        }

        side_to_move = (active_color == "w") ? Color::WHITE : Color::BLACK;
        if (side_to_move == Color::BLACK) hash_key ^= Zobrist::side_key;

        if (castling != "-") {
            if (castling.find('K') != std::string::npos) castling_rights |= WK;
            if (castling.find('Q') != std::string::npos) castling_rights |= WQ;
            if (castling.find('k') != std::string::npos) castling_rights |= BK;
            if (castling.find('q') != std::string::npos) castling_rights |= BQ;
        }
        // XOR the combined 0–15 rights index so FEN hashes match make_move updates
        hash_key ^= Zobrist::castling_keys[castling_rights];

        if (en_passant != "-") {
            int f = en_passant[0] - 'a';
            int r = en_passant[1] - '1';
            en_passant_sq = static_cast<Square>(r * 8 + f);
            hash_key ^= Zobrist::enpassant_keys[static_cast<int>(en_passant_sq)];
        }

        if (!half_move.empty()) half_move_clock = std::stoi(half_move);
        if (!full_move.empty()) full_move_number = std::stoi(full_move);
        return true;
    }

    void Board::make_move(const Move& move) {
        // Push current state onto history so we can fully restore it
        State s;
        s.castling_rights = castling_rights;
        s.en_passant_sq   = en_passant_sq;
        s.half_move_clock = half_move_clock;
        s.captured_piece  = Piece::NONE;
        s.hash_key        = hash_key;
        history.push_back(s);

        Square from = move.get_from();
        Square to   = move.get_to();
        MoveFlag flag = move.get_flag();

        Piece moving = piece_on(from);
        Color us   = side_to_move;
        Color them = (us == Color::WHITE) ? Color::BLACK : Color::WHITE;

        // Clear old en passant hash
        if (en_passant_sq != Square::NONE)
            hash_key ^= Zobrist::enpassant_keys[static_cast<int>(en_passant_sq)];
        en_passant_sq = Square::NONE;

        // Handle captures, including promotion captures (EP is a special case below)
        if (move.is_capture() && flag != MoveFlag::EnPassant) {
            Piece captured = piece_on(to);
            history.back().captured_piece = captured;
            remove_piece_at(captured, them, to);
        }

        // Handle En Passant capture (captured pawn is NOT on 'to')
        if (flag == MoveFlag::EnPassant) {
            int ep_pawn_sq = (us == Color::WHITE) ? static_cast<int>(to) - 8 : static_cast<int>(to) + 8;
            remove_piece_at(Piece::PAWN, them, static_cast<Square>(ep_pawn_sq));
            history.back().captured_piece = Piece::PAWN;
        }

        // Move the piece
        remove_piece_at(moving, us, from);
        set_piece_at(moving, us, to);

        // Handle Promotions: replace Pawn with promo piece
        if (move.is_promotion()) {
            remove_piece_at(Piece::PAWN, us, to);
            set_piece_at(move.promo_piece(), us, to);
        }

        // Handle Castling: move the Rook
        if (flag == MoveFlag::KingCastle) {
            if (us == Color::WHITE) { remove_piece_at(Piece::ROOK, us, Square::H1); set_piece_at(Piece::ROOK, us, Square::F1); }
            else                    { remove_piece_at(Piece::ROOK, us, Square::H8); set_piece_at(Piece::ROOK, us, Square::F8); }
        }
        if (flag == MoveFlag::QueenCastle) {
            if (us == Color::WHITE) { remove_piece_at(Piece::ROOK, us, Square::A1); set_piece_at(Piece::ROOK, us, Square::D1); }
            else                    { remove_piece_at(Piece::ROOK, us, Square::A8); set_piece_at(Piece::ROOK, us, Square::D8); }
        }

        // Update castling rights if King or Rook moved
        hash_key ^= Zobrist::castling_keys[castling_rights];
        if (moving == Piece::KING) {
            if (us == Color::WHITE) castling_rights &= ~(WK | WQ);
            else                    castling_rights &= ~(BK | BQ);
        }
        if (from == Square::A1 || to == Square::A1) castling_rights &= ~WQ;
        if (from == Square::H1 || to == Square::H1) castling_rights &= ~WK;
        if (from == Square::A8 || to == Square::A8) castling_rights &= ~BQ;
        if (from == Square::H8 || to == Square::H8) castling_rights &= ~BK;
        hash_key ^= Zobrist::castling_keys[castling_rights];

        // Set new en passant square for double pawn pushes
        if (flag == MoveFlag::DoublePawnPush) {
            en_passant_sq = static_cast<Square>((static_cast<int>(from) + static_cast<int>(to)) / 2);
            hash_key ^= Zobrist::enpassant_keys[static_cast<int>(en_passant_sq)];
        }

        // Flip side to move
        side_to_move = them;
        hash_key ^= Zobrist::side_key;

        half_move_clock++;
        if (moving == Piece::PAWN || move.is_capture()) half_move_clock = 0;
        if (us == Color::BLACK) full_move_number++;
    }

    void Board::unmake_move(const Move& move) {
        if (history.empty()) return;

        State& s = history.back();

        Square from  = move.get_from();
        Square to    = move.get_to();
        MoveFlag flag = move.get_flag();

        // Flip side BACK
        Color us   = (side_to_move == Color::WHITE) ? Color::BLACK : Color::WHITE;
        Color them = side_to_move;
        side_to_move = us;

        Piece moving = piece_on(to);

        // Reverse promotion: swap promo piece back to pawn
        if (move.is_promotion()) {
            remove_piece_at(move.promo_piece(), us, to);
            set_piece_at(Piece::PAWN, us, to);
            moving = Piece::PAWN;
        }

        // Move piece back
        remove_piece_at(moving, us, to);
        set_piece_at(moving, us, from);

        // Restore captured piece (including promotion captures)
        if (move.is_capture() && flag != MoveFlag::EnPassant && s.captured_piece != Piece::NONE) {
            set_piece_at(s.captured_piece, them, to);
        }

        // Restore en passant captured pawn
        if (flag == MoveFlag::EnPassant) {
            int ep_pawn_sq = (us == Color::WHITE) ? static_cast<int>(to) - 8 : static_cast<int>(to) + 8;
            set_piece_at(Piece::PAWN, them, static_cast<Square>(ep_pawn_sq));
        }

        // Restore castling rook
        if (flag == MoveFlag::KingCastle) {
            if (us == Color::WHITE) { remove_piece_at(Piece::ROOK, us, Square::F1); set_piece_at(Piece::ROOK, us, Square::H1); }
            else                    { remove_piece_at(Piece::ROOK, us, Square::F8); set_piece_at(Piece::ROOK, us, Square::H8); }
        }
        if (flag == MoveFlag::QueenCastle) {
            if (us == Color::WHITE) { remove_piece_at(Piece::ROOK, us, Square::D1); set_piece_at(Piece::ROOK, us, Square::A1); }
            else                    { remove_piece_at(Piece::ROOK, us, Square::D8); set_piece_at(Piece::ROOK, us, Square::A8); }
        }

        // Restore previous state
        hash_key       = s.hash_key;
        castling_rights = s.castling_rights;
        en_passant_sq  = s.en_passant_sq;
        half_move_clock = s.half_move_clock;

        if (us == Color::BLACK) full_move_number--;
        history.pop_back();
    }

    void Board::print() const {
        std::cout << "\n";
        for (int rank = 7; rank >= 0; rank--) {
            std::cout << rank + 1 << "  ";
            for (int file = 0; file < 8; file++) {
                Square sq = static_cast<Square>(rank * 8 + file);
                char pc = '.';
                if (get_bit(color_bbs[0], sq)) {
                    if (get_bit(piece_bbs[0],sq)) pc='P'; else if (get_bit(piece_bbs[1],sq)) pc='N';
                    else if (get_bit(piece_bbs[2],sq)) pc='B'; else if (get_bit(piece_bbs[3],sq)) pc='R';
                    else if (get_bit(piece_bbs[4],sq)) pc='Q'; else pc='K';
                } else if (get_bit(color_bbs[1], sq)) {
                    if (get_bit(piece_bbs[0],sq)) pc='p'; else if (get_bit(piece_bbs[1],sq)) pc='n';
                    else if (get_bit(piece_bbs[2],sq)) pc='b'; else if (get_bit(piece_bbs[3],sq)) pc='r';
                    else if (get_bit(piece_bbs[4],sq)) pc='q'; else pc='k';
                }
                std::cout << pc << " ";
            }
            std::cout << "\n";
        }
        std::cout << "   a b c d e f g h\n\n";
        std::cout << "Side: " << (side_to_move == Color::WHITE ? "White" : "Black")
                  << "  Castling: " << castling_rights
                  << "  EP: " << (en_passant_sq != Square::NONE ? static_cast<int>(en_passant_sq) : -1) << "\n";
    }
}
