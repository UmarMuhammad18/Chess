#pragma once
#include "bitboard.hpp"
#include <array>
#include <string>
#include <vector>

namespace engine {
/*
 * Represents the reversible state saved before a move is made.
 * Includes castling rights, en‑passant square, half‑move clock,
 * captured piece information and the Zobrist hash key.
 */
    struct State {
        int      castling_rights;
        Square   en_passant_sq;
        int      half_move_clock;
        Piece    captured_piece;
        U64      hash_key;
    };

/*
 * Board class encapsulates the chess board state, piece bitboards,
 * side to move, castling rights, etc. Provides methods to load FEN,
 * make/unmake moves, and query piece locations.
 */
    public:
        Board();
        void init();
        bool load_fen(const std::string& fen);
        void print() const;

        void make_move(const Move& move);
        void unmake_move(const Move& move);

        // Accessors
        U64   get_pieces(Piece p) const { return piece_bbs[static_cast<int>(p)]; }
        U64   get_pieces(Color c) const { return color_bbs[static_cast<int>(c)]; }
        U64   get_pieces(Piece p, Color c) const { return get_pieces(p) & get_pieces(c); }
        Color get_side_to_move() const { return side_to_move; }
        int   get_castling_rights() const { return castling_rights; }
        Square get_en_passant_sq() const { return en_passant_sq; }
        U64   get_occupancy() const { return color_bbs[0] | color_bbs[1]; }

        // Internal helpers (used by movegen and tests)
        void set_piece_at(Piece p, Color c, Square sq);
        void remove_piece_at(Piece p, Color c, Square sq);

        Piece piece_on(Square sq) const;
        Color color_on(Square sq) const;

        // Hash key (public for search)
        U64 hash_key{0};

    private:
        std::array<U64, 6> piece_bbs;
        std::array<U64, 2> color_bbs;

        Color  side_to_move;
        Square en_passant_sq;
        int    castling_rights;
        int    half_move_clock;
        int    full_move_number;

        std::vector<State> history;
    };
}
