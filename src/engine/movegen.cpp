#include "movegen.hpp"
#include "attacks.hpp"
#include "utils.hpp"

namespace engine {
    namespace {
        constexpr U64 bit(Square sq) { return 1ULL << static_cast<int>(sq); }

        void add_promotions(MoveList& moves, Square from, Square to, bool capture, bool queen_only) {
            if (capture) {
                moves.push_back(Move(from, to, MoveFlag::PromoQueenCap));
                if (queen_only) return;
                moves.push_back(Move(from, to, MoveFlag::PromoRookCap));
                moves.push_back(Move(from, to, MoveFlag::PromoBishopCap));
                moves.push_back(Move(from, to, MoveFlag::PromoKnightCap));
            } else {
                moves.push_back(Move(from, to, MoveFlag::PromoQueen));
                if (queen_only) return;
                moves.push_back(Move(from, to, MoveFlag::PromoRook));
                moves.push_back(Move(from, to, MoveFlag::PromoBishop));
                moves.push_back(Move(from, to, MoveFlag::PromoKnight));
            }
        }

        void add_piece_moves(MoveList& moves, Square from, U64 targets, U64 their) {
            while (targets) {
                Square to = static_cast<Square>(pop_lsb(targets));
                moves.push_back(Move(from, to, (their & bit(to)) ? MoveFlag::Capture : MoveFlag::Quiet));
            }
        }
    }

    void MoveGen::init() {
        attacks::init();
    }

    // -----------------------------------------------------------------------
    bool MoveGen::is_square_attacked(const Board& board, Square sq, Color by_color) {
        // A pawn of by_color attacks sq iff a pawn of the other color on sq would attack it
        if (attacks::pawn(opposite(by_color), sq) & board.get_pieces(Piece::PAWN, by_color)) return true;
        if (attacks::knight(sq) & board.get_pieces(Piece::KNIGHT, by_color)) return true;
        if (attacks::king(sq) & board.get_pieces(Piece::KING, by_color)) return true;

        U64 occ    = board.get_occupancy();
        U64 queens = board.get_pieces(Piece::QUEEN, by_color);
        if (attacks::bishop(sq, occ) & (board.get_pieces(Piece::BISHOP, by_color) | queens)) return true;
        if (attacks::rook(sq, occ)   & (board.get_pieces(Piece::ROOK, by_color)   | queens)) return true;
        return false;
    }

    bool MoveGen::is_king_attacked(const Board& board, Color c) {
        U64 king_bb = board.get_pieces(Piece::KING, c);
        if (!king_bb) return false;
        return is_square_attacked(board, static_cast<Square>(lsb(king_bb)), opposite(c));
    }

    // -----------------------------------------------------------------------
    template <bool CapturesOnly>
    void MoveGen::gen_pawn_moves(const Board& board, MoveList& moves, Color us) {
        Color them = opposite(us);
        U64 occ    = board.get_occupancy();
        U64 their  = board.get_pieces(them);
        U64 pawns  = board.get_pieces(Piece::PAWN, us);
        bool white = us == Color::WHITE;
        int  up    = white ? 8 : -8;

        // Destination rank of a promoting push: 8th for White, 1st for Black
        U64 promo_rank = white ? 0xFF00000000000000ULL : 0x00000000000000FFULL;

        U64 push1 = (white ? (pawns << 8) : (pawns >> 8)) & ~occ;

        // Promotions via push (queen only in captures mode)
        U64 push1_promo = push1 & promo_rank;
        while (push1_promo) {
            int to_sq = pop_lsb(push1_promo);
            add_promotions(moves, static_cast<Square>(to_sq - up), static_cast<Square>(to_sq), false, CapturesOnly);
        }

        if constexpr (!CapturesOnly) {
            U64 push2 = white ? ((push1 & 0x0000000000FF0000ULL) << 8) & ~occ
                              : ((push1 & 0x0000FF0000000000ULL) >> 8) & ~occ;

            U64 push1_np = push1 & ~promo_rank;
            while (push1_np) {
                int to_sq = pop_lsb(push1_np);
                moves.push_back(Move(static_cast<Square>(to_sq - up), static_cast<Square>(to_sq), MoveFlag::Quiet));
            }
            while (push2) {
                int to_sq = pop_lsb(push2);
                moves.push_back(Move(static_cast<Square>(to_sq - 2 * up), static_cast<Square>(to_sq), MoveFlag::DoublePawnPush));
            }
        }

        // Captures, capture-promotions and en passant
        Square ep = board.get_en_passant_sq();
        while (pawns) {
            Square from = static_cast<Square>(pop_lsb(pawns));
            U64 pawn_att = attacks::pawn(us, from);
            U64 targets  = pawn_att & their;
            while (targets) {
                Square to = static_cast<Square>(pop_lsb(targets));
                if (bit(to) & promo_rank) add_promotions(moves, from, to, true, CapturesOnly);
                else                      moves.push_back(Move(from, to, MoveFlag::Capture));
            }
            if (ep != Square::NONE && (pawn_att & bit(ep))) {
                moves.push_back(Move(from, ep, MoveFlag::EnPassant));
            }
        }
    }

    void MoveGen::gen_castling_moves(const Board& board, MoveList& moves, Color us) {
        int rights = board.get_castling_rights();
        Color them = opposite(us);
        U64 occ    = board.get_occupancy();

        bool white = us == Color::WHITE;
        int  ks    = white ? WK : BK;
        int  qs    = white ? WQ : BQ;
        Square e = white ? Square::E1 : Square::E8;
        Square f = white ? Square::F1 : Square::F8;
        Square g = white ? Square::G1 : Square::G8;
        Square d = white ? Square::D1 : Square::D8;
        Square c = white ? Square::C1 : Square::C8;
        Square b = white ? Square::B1 : Square::B8;

        if (!(rights & (ks | qs)) || is_square_attacked(board, e, them)) return;

        if ((rights & ks) && !(occ & (bit(f) | bit(g))) &&
            !is_square_attacked(board, f, them) && !is_square_attacked(board, g, them)) {
            moves.push_back(Move(e, g, MoveFlag::KingCastle));
        }
        if ((rights & qs) && !(occ & (bit(b) | bit(c) | bit(d))) &&
            !is_square_attacked(board, d, them) && !is_square_attacked(board, c, them)) {
            moves.push_back(Move(e, c, MoveFlag::QueenCastle));
        }
    }

    template <bool CapturesOnly>
    void MoveGen::generate(const Board& board, MoveList& moves) {
        Color us   = board.get_side_to_move();
        U64 our    = board.get_pieces(us);
        U64 their  = board.get_pieces(opposite(us));
        U64 occ    = our | their;
        U64 target = CapturesOnly ? their : ~our;

        gen_pawn_moves<CapturesOnly>(board, moves, us);

        U64 knights = board.get_pieces(Piece::KNIGHT, us);
        while (knights) {
            Square sq = static_cast<Square>(pop_lsb(knights));
            add_piece_moves(moves, sq, attacks::knight(sq) & target, their);
        }
        U64 bishops = board.get_pieces(Piece::BISHOP, us);
        while (bishops) {
            Square sq = static_cast<Square>(pop_lsb(bishops));
            add_piece_moves(moves, sq, attacks::bishop(sq, occ) & target, their);
        }
        U64 rooks = board.get_pieces(Piece::ROOK, us);
        while (rooks) {
            Square sq = static_cast<Square>(pop_lsb(rooks));
            add_piece_moves(moves, sq, attacks::rook(sq, occ) & target, their);
        }
        U64 queens = board.get_pieces(Piece::QUEEN, us);
        while (queens) {
            Square sq = static_cast<Square>(pop_lsb(queens));
            add_piece_moves(moves, sq, attacks::queen(sq, occ) & target, their);
        }
        U64 king = board.get_pieces(Piece::KING, us);
        if (king) {
            Square sq = static_cast<Square>(lsb(king));
            add_piece_moves(moves, sq, attacks::king(sq) & target, their);
        }

        if constexpr (!CapturesOnly) gen_castling_moves(board, moves, us);
    }

    void MoveGen::generate_pseudo_legal_moves(const Board& board, MoveList& moves) {
        generate<false>(board, moves);
    }

    void MoveGen::generate_captures(const Board& board, MoveList& moves) {
        generate<true>(board, moves);
    }

    // -----------------------------------------------------------------------
    // Legal move filter: make the move, check if our king is attacked, unmake
    MoveList MoveGen::generate_legal_moves(Board& board) {
        MoveList pseudo;
        generate_pseudo_legal_moves(board, pseudo);

        MoveList legal;
        Color us = board.get_side_to_move();
        for (const Move& m : pseudo) {
            board.make_move(m);
            if (!is_king_attacked(board, us)) legal.push_back(m);
            board.unmake_move(m);
        }
        return legal;
    }
}
