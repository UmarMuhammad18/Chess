#include "board.hpp"
#include "utils.hpp"
#include "zobrist.hpp"
#include <cctype>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace engine {
    namespace {
        constexpr char PIECE_CHARS[] = "pnbrqk";
    }

    Board::Board() {
        Zobrist::init();
        init();
    }

    void Board::init() {
        piece_bbs.fill(0);
        color_bbs.fill(0);
        mailbox.fill(Piece::NONE);
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
        mailbox[static_cast<int>(sq)] = p;
        hash_key ^= Zobrist::piece_keys[static_cast<int>(p)][static_cast<int>(c)][static_cast<int>(sq)];
    }

    void Board::remove_piece_at(Piece p, Color c, Square sq) {
        clear_bit(piece_bbs[static_cast<int>(p)], sq);
        clear_bit(color_bbs[static_cast<int>(c)], sq);
        mailbox[static_cast<int>(sq)] = Piece::NONE;
        hash_key ^= Zobrist::piece_keys[static_cast<int>(p)][static_cast<int>(c)][static_cast<int>(sq)];
    }

    Color Board::color_on(Square sq) const {
        if (get_bit(color_bbs[0], sq)) return Color::WHITE;
        if (get_bit(color_bbs[1], sq)) return Color::BLACK;
        return Color::BOTH;
    }

    bool Board::has_non_pawn_material(Color c) const {
        return (get_pieces(c) & ~get_pieces(Piece::PAWN) & ~get_pieces(Piece::KING)) != 0;
    }

    bool Board::load_fen(const std::string& fen) {
        Board b;
        std::istringstream iss(fen);
        std::string board_part, active_color, castling, en_passant, half_move, full_move;
        iss >> board_part >> active_color >> castling >> en_passant >> half_move >> full_move;

        int rank = 7, file = 0;
        for (char c : board_part) {
            if (c == '/') {
                if (file != 8 || rank == 0) return false;
                rank--; file = 0;
            } else if (c >= '1' && c <= '8') {
                file += (c - '0');
                if (file > 8) return false;
            } else {
                const char* p = std::char_traits<char>::find(PIECE_CHARS, 6, static_cast<char>(tolower(static_cast<unsigned char>(c))));
                if (!p || file > 7) return false;
                Color col = isupper(static_cast<unsigned char>(c)) ? Color::WHITE : Color::BLACK;
                b.set_piece_at(static_cast<Piece>(p - PIECE_CHARS), col, static_cast<Square>(rank * 8 + file));
                file++;
            }
        }
        if (rank != 0 || file != 8) return false;
        if (popcount(b.get_pieces(Piece::KING, Color::WHITE)) != 1 ||
            popcount(b.get_pieces(Piece::KING, Color::BLACK)) != 1) return false;

        if (active_color == "w")      b.side_to_move = Color::WHITE;
        else if (active_color == "b") b.side_to_move = Color::BLACK;
        else return false;
        if (b.side_to_move == Color::BLACK) b.hash_key ^= Zobrist::side_key;

        if (!castling.empty() && castling != "-") {
            for (char c : castling) {
                switch (c) {
                    case 'K': b.castling_rights |= WK; break;
                    case 'Q': b.castling_rights |= WQ; break;
                    case 'k': b.castling_rights |= BK; break;
                    case 'q': b.castling_rights |= BQ; break;
                    default: return false;
                }
            }
        }
        // XOR the combined 0–15 rights index so FEN hashes match make_move updates
        b.hash_key ^= Zobrist::castling_keys[b.castling_rights];

        if (!en_passant.empty() && en_passant != "-") {
            if (en_passant.size() != 2) return false;
            int f = en_passant[0] - 'a';
            int r = en_passant[1] - '1';
            if (f < 0 || f > 7 || (r != 2 && r != 5)) return false;
            b.en_passant_sq = static_cast<Square>(r * 8 + f);
            b.hash_key ^= Zobrist::enpassant_keys[static_cast<int>(b.en_passant_sq)];
        }

        try {
            if (!half_move.empty()) b.half_move_clock = std::stoi(half_move);
            if (!full_move.empty()) b.full_move_number = std::stoi(full_move);
        } catch (const std::exception&) {
            return false;
        }

        *this = b;
        return true;
    }

    std::string Board::get_fen() const {
        std::string fen;
        for (int rank = 7; rank >= 0; rank--) {
            int empty = 0;
            for (int file = 0; file < 8; file++) {
                Square sq = static_cast<Square>(rank * 8 + file);
                Piece p = piece_on(sq);
                if (p == Piece::NONE) { empty++; continue; }
                if (empty) { fen += static_cast<char>('0' + empty); empty = 0; }
                char c = PIECE_CHARS[static_cast<int>(p)];
                fen += color_on(sq) == Color::WHITE ? static_cast<char>(toupper(c)) : c;
            }
            if (empty) fen += static_cast<char>('0' + empty);
            if (rank) fen += '/';
        }
        fen += side_to_move == Color::WHITE ? " w " : " b ";
        if (!castling_rights) fen += '-';
        if (castling_rights & WK) fen += 'K';
        if (castling_rights & WQ) fen += 'Q';
        if (castling_rights & BK) fen += 'k';
        if (castling_rights & BQ) fen += 'q';
        fen += ' ';
        if (en_passant_sq == Square::NONE) {
            fen += '-';
        } else {
            fen += static_cast<char>('a' + static_cast<int>(en_passant_sq) % 8);
            fen += static_cast<char>('1' + static_cast<int>(en_passant_sq) / 8);
        }
        fen += ' ' + std::to_string(half_move_clock) + ' ' + std::to_string(full_move_number);
        return fen;
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
        Color them = opposite(us);

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

        // Move the piece (promotions place the new piece directly)
        remove_piece_at(moving, us, from);
        set_piece_at(move.is_promotion() ? move.promo_piece() : moving, us, to);

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
        Color us   = opposite(side_to_move);
        Color them = side_to_move;
        side_to_move = us;

        // Move piece back (a promoted piece turns back into a pawn)
        Piece moved = piece_on(to);
        remove_piece_at(moved, us, to);
        set_piece_at(move.is_promotion() ? Piece::PAWN : moved, us, from);

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

    void Board::make_null_move() {
        history.push_back(State{castling_rights, en_passant_sq, half_move_clock, Piece::NONE, hash_key});

        if (en_passant_sq != Square::NONE)
            hash_key ^= Zobrist::enpassant_keys[static_cast<int>(en_passant_sq)];
        en_passant_sq = Square::NONE;

        side_to_move = opposite(side_to_move);
        hash_key ^= Zobrist::side_key;
        // Positions before a null move can't legally recur after it
        half_move_clock = 0;
    }

    void Board::unmake_null_move() {
        const State& s = history.back();
        side_to_move    = opposite(side_to_move);
        hash_key        = s.hash_key;
        en_passant_sq   = s.en_passant_sq;
        half_move_clock = s.half_move_clock;
        history.pop_back();
    }

    int Board::repetition_count() const {
        // history[i].hash_key is the position before move i; only positions with the
        // same side to move (every other entry) inside the reversible window can match
        int count = 0;
        int n = static_cast<int>(history.size());
        for (int i = n - 2; i >= 0 && i >= n - half_move_clock; i -= 2) {
            if (history[i].hash_key == hash_key) count++;
        }
        return count;
    }

    bool Board::is_insufficient_material() const {
        if (get_pieces(Piece::PAWN) | get_pieces(Piece::ROOK) | get_pieces(Piece::QUEEN)) return false;
        // K vs K, or K + single minor vs K
        return popcount(get_pieces(Piece::KNIGHT) | get_pieces(Piece::BISHOP)) <= 1;
    }

    void Board::print() const {
        std::cout << "\n";
        for (int rank = 7; rank >= 0; rank--) {
            std::cout << rank + 1 << "  ";
            for (int file = 0; file < 8; file++) {
                Square sq = static_cast<Square>(rank * 8 + file);
                char pc = '.';
                Piece p = piece_on(sq);
                if (p != Piece::NONE) {
                    pc = PIECE_CHARS[static_cast<int>(p)];
                    if (color_on(sq) == Color::WHITE) pc = static_cast<char>(toupper(pc));
                }
                std::cout << pc << " ";
            }
            std::cout << "\n";
        }
        std::cout << "   a b c d e f g h\n\n";
        std::cout << "FEN: " << get_fen() << "\n";
        std::cout << "Key: " << std::hex << hash_key << std::dec << "\n";
    }
}
