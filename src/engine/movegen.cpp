#include "movegen.hpp"
#include "utils.hpp"

namespace engine {
    U64 MoveGen::knight_attacks[64];
    U64 MoveGen::king_attacks[64];
    U64 MoveGen::pawn_attacks[2][64];

    const int dir_offsets[8] = {8, -8, 1, -1, 9, 7, -7, -9};

    void MoveGen::init() {
        for (int sq = 0; sq < 64; sq++) {
            U64 bb = 1ULL << sq;

            // Pawn attacks
            pawn_attacks[0][sq] = ((bb << 7) & ~0x8080808080808080ULL) | ((bb << 9) & ~0x0101010101010101ULL);
            pawn_attacks[1][sq] = ((bb >> 7) & ~0x0101010101010101ULL) | ((bb >> 9) & ~0x8080808080808080ULL);

            // Knight attacks
            U64 n = 0;
            n |= (bb << 17) & ~0x0101010101010101ULL;
            n |= (bb << 15) & ~0x8080808080808080ULL;
            n |= (bb << 10) & ~0x0303030303030303ULL;
            n |= (bb << 6)  & ~0xC0C0C0C0C0C0C0C0ULL;
            n |= (bb >> 17) & ~0x8080808080808080ULL;
            n |= (bb >> 15) & ~0x0101010101010101ULL;
            n |= (bb >> 10) & ~0xC0C0C0C0C0C0C0C0ULL;
            n |= (bb >> 6)  & ~0x0303030303030303ULL;
            knight_attacks[sq] = n;

            // King attacks
            U64 k = ((bb << 1) & ~0x0101010101010101ULL) | ((bb >> 1) & ~0x8080808080808080ULL);
            k |= bb << 8 | bb >> 8;
            k |= ((bb << 9)  & ~0x0101010101010101ULL) | ((bb >> 9)  & ~0x8080808080808080ULL);
            k |= ((bb << 7)  & ~0x8080808080808080ULL) | ((bb >> 7)  & ~0x0101010101010101ULL);
            king_attacks[sq] = k;
        }
    }

    // -----------------------------------------------------------------------
    U64 MoveGen::get_ray_attacks(Square sq, U64 occupancy, int dir_index) {
        U64 attacks = 0;
        int s = static_cast<int>(sq);
        const int d = dir_offsets[dir_index];
        while (true) {
            int ns = s + d;
            if (ns < 0 || ns >= 64) break;
            // Wrap guard: horizontal moves can't wrap across file A/H
            if ((d == 1 || d == 9 || d == -7) && (s % 8 == 7)) break;
            if ((d == -1 || d == -9 || d == 7) && (s % 8 == 0)) break;
            attacks |= (1ULL << ns);
            if (occupancy & (1ULL << ns)) break;
            s = ns;
        }
        return attacks;
    }

    U64 MoveGen::rook_attacks_otf(Square sq, U64 occ) {
        return get_ray_attacks(sq, occ, 0) | get_ray_attacks(sq, occ, 1) |
               get_ray_attacks(sq, occ, 2) | get_ray_attacks(sq, occ, 3);
    }

    U64 MoveGen::bishop_attacks_otf(Square sq, U64 occ) {
        return get_ray_attacks(sq, occ, 4) | get_ray_attacks(sq, occ, 5) |
               get_ray_attacks(sq, occ, 6) | get_ray_attacks(sq, occ, 7);
    }

    // -----------------------------------------------------------------------
    bool MoveGen::is_square_attacked(const Board& board, Square sq, Color by_color) {
        int sqIdx = static_cast<int>(sq);

        // Pawns
        U64 pawns = board.get_pieces(Piece::PAWN, by_color);
        int pc = (by_color == Color::WHITE) ? 0 : 1;
        if (pawn_attacks[1 - pc][sqIdx] & pawns) return true;

        // Knights
        if (knight_attacks[sqIdx] & board.get_pieces(Piece::KNIGHT, by_color)) return true;

        // King
        if (king_attacks[sqIdx] & board.get_pieces(Piece::KING, by_color)) return true;

        // Sliders
        U64 occ = board.get_occupancy();
        if (bishop_attacks_otf(sq, occ) &
            (board.get_pieces(Piece::BISHOP, by_color) | board.get_pieces(Piece::QUEEN, by_color)))
            return true;
        if (rook_attacks_otf(sq, occ) &
            (board.get_pieces(Piece::ROOK, by_color) | board.get_pieces(Piece::QUEEN, by_color)))
            return true;

        return false;
    }

    // -----------------------------------------------------------------------
    void MoveGen::gen_pawn_moves(const Board& board, std::vector<Move>& moves, Color us) {
        Color them = (us == Color::WHITE) ? Color::BLACK : Color::WHITE;
        U64 occ    = board.get_occupancy();
        U64 their  = board.get_pieces(them);
        U64 pawns  = board.get_pieces(Piece::PAWN, us);

        // Destination rank of a promoting push: 8th for White, 1st for Black
        U64 promo_rank = (us == Color::WHITE) ? 0xFF00000000000000ULL : 0x00000000000000FFULL;

        // Single pushes
        U64 push1 = (us == Color::WHITE) ? (pawns << 8) & ~occ : (pawns >> 8) & ~occ;
        // Double pushes from rank 2
        U64 push2 = (us == Color::WHITE) ? ((push1 & (0x0000000000FF0000ULL)) << 8) & ~occ
                                         : ((push1 & (0x0000FF0000000000ULL)) >> 8) & ~occ;

        // Non-promotion pushes
        U64 push1_np = push1 & ~promo_rank;
        while (push1_np) {
            int to_sq = pop_lsb(push1_np);
            int from_sq = (us == Color::WHITE) ? to_sq - 8 : to_sq + 8;
            moves.push_back(Move(static_cast<Square>(from_sq), static_cast<Square>(to_sq), MoveFlag::Quiet));
        }
        // Promotions via push
        U64 push1_promo = push1 & promo_rank;
        while (push1_promo) {
            int to_sq = pop_lsb(push1_promo);
            int from_sq = (us == Color::WHITE) ? to_sq - 8 : to_sq + 8;
            Square f = static_cast<Square>(from_sq), t = static_cast<Square>(to_sq);
            moves.push_back(Move(f, t, MoveFlag::PromoQueen));
            moves.push_back(Move(f, t, MoveFlag::PromoRook));
            moves.push_back(Move(f, t, MoveFlag::PromoBishop));
            moves.push_back(Move(f, t, MoveFlag::PromoKnight));
        }
        // Double pushes
        while (push2) {
            int to_sq = pop_lsb(push2);
            int from_sq = (us == Color::WHITE) ? to_sq - 16 : to_sq + 16;
            moves.push_back(Move(static_cast<Square>(from_sq), static_cast<Square>(to_sq), MoveFlag::DoublePawnPush));
        }

        // Pawn captures
        U64 pawns2 = pawns;
        while (pawns2) {
            int from_sq = pop_lsb(pawns2);
            Square from = static_cast<Square>(from_sq);
            U64 attacks = pawn_attacks[static_cast<int>(us)][from_sq] & their;
            while (attacks) {
                int to_sq = pop_lsb(attacks);
                Square to = static_cast<Square>(to_sq);
                bool is_promo = (us == Color::WHITE) ? (to_sq >= 56) : (to_sq < 8);
                if (is_promo) {
                    moves.push_back(Move(from, to, MoveFlag::PromoQueenCap));
                    moves.push_back(Move(from, to, MoveFlag::PromoRookCap));
                    moves.push_back(Move(from, to, MoveFlag::PromoBishopCap));
                    moves.push_back(Move(from, to, MoveFlag::PromoKnightCap));
                } else {
                    moves.push_back(Move(from, to, MoveFlag::Capture));
                }
            }
            // En Passant
            Square ep = board.get_en_passant_sq();
            if (ep != Square::NONE) {
                if (pawn_attacks[static_cast<int>(us)][from_sq] & (1ULL << static_cast<int>(ep))) {
                    moves.push_back(Move(from, ep, MoveFlag::EnPassant));
                }
            }
        }
    }

    void MoveGen::gen_castling_moves(const Board& board, std::vector<Move>& moves, Color us) {
        int rights = board.get_castling_rights();
        Color them = (us == Color::WHITE) ? Color::BLACK : Color::WHITE;
        U64 occ    = board.get_occupancy();

        if (us == Color::WHITE) {
            // King-side
            if ((rights & WK) &&
                !(occ & ((1ULL << static_cast<int>(Square::F1)) | (1ULL << static_cast<int>(Square::G1)))) &&
                !is_square_attacked(board, Square::E1, them) &&
                !is_square_attacked(board, Square::F1, them) &&
                !is_square_attacked(board, Square::G1, them))
            {
                moves.push_back(Move(Square::E1, Square::G1, MoveFlag::KingCastle));
            }
            // Queen-side
            if ((rights & WQ) &&
                !(occ & ((1ULL<<static_cast<int>(Square::B1))|(1ULL<<static_cast<int>(Square::C1))|(1ULL<<static_cast<int>(Square::D1)))) &&
                !is_square_attacked(board, Square::E1, them) &&
                !is_square_attacked(board, Square::D1, them) &&
                !is_square_attacked(board, Square::C1, them))
            {
                moves.push_back(Move(Square::E1, Square::C1, MoveFlag::QueenCastle));
            }
        } else {
            if ((rights & BK) &&
                !(occ & ((1ULL << static_cast<int>(Square::F8)) | (1ULL << static_cast<int>(Square::G8)))) &&
                !is_square_attacked(board, Square::E8, them) &&
                !is_square_attacked(board, Square::F8, them) &&
                !is_square_attacked(board, Square::G8, them))
            {
                moves.push_back(Move(Square::E8, Square::G8, MoveFlag::KingCastle));
            }
            if ((rights & BQ) &&
                !(occ & ((1ULL<<static_cast<int>(Square::B8))|(1ULL<<static_cast<int>(Square::C8))|(1ULL<<static_cast<int>(Square::D8)))) &&
                !is_square_attacked(board, Square::E8, them) &&
                !is_square_attacked(board, Square::D8, them) &&
                !is_square_attacked(board, Square::C8, them))
            {
                moves.push_back(Move(Square::E8, Square::C8, MoveFlag::QueenCastle));
            }
        }
    }

    std::vector<Move> MoveGen::generate_pseudo_legal_moves(const Board& board) {
        std::vector<Move> moves;
        moves.reserve(64);

        Color us   = board.get_side_to_move();
        Color them = (us == Color::WHITE) ? Color::BLACK : Color::WHITE;
        U64 our    = board.get_pieces(us);
        U64 their  = board.get_pieces(them);
        U64 occ    = our | their;

        // Pawns (with EP, promotions, and double-pushes)
        gen_pawn_moves(board, moves, us);

        // Knights
        U64 knights = board.get_pieces(Piece::KNIGHT, us);
        while (knights) {
            Square sq = static_cast<Square>(pop_lsb(knights));
            U64 attacks = knight_attacks[static_cast<int>(sq)] & ~our;
            while (attacks) {
                Square to = static_cast<Square>(pop_lsb(attacks));
                MoveFlag flag = get_bit(their, to) ? MoveFlag::Capture : MoveFlag::Quiet;
                moves.push_back(Move(sq, to, flag));
            }
        }

        // Bishops
        U64 bishops = board.get_pieces(Piece::BISHOP, us);
        while (bishops) {
            Square sq = static_cast<Square>(pop_lsb(bishops));
            U64 attacks = bishop_attacks_otf(sq, occ) & ~our;
            while (attacks) {
                Square to = static_cast<Square>(pop_lsb(attacks));
                moves.push_back(Move(sq, to, get_bit(their, to) ? MoveFlag::Capture : MoveFlag::Quiet));
            }
        }

        // Rooks
        U64 rooks = board.get_pieces(Piece::ROOK, us);
        while (rooks) {
            Square sq = static_cast<Square>(pop_lsb(rooks));
            U64 attacks = rook_attacks_otf(sq, occ) & ~our;
            while (attacks) {
                Square to = static_cast<Square>(pop_lsb(attacks));
                moves.push_back(Move(sq, to, get_bit(their, to) ? MoveFlag::Capture : MoveFlag::Quiet));
            }
        }

        // Queens
        U64 queens = board.get_pieces(Piece::QUEEN, us);
        while (queens) {
            Square sq = static_cast<Square>(pop_lsb(queens));
            U64 attacks = (bishop_attacks_otf(sq, occ) | rook_attacks_otf(sq, occ)) & ~our;
            while (attacks) {
                Square to = static_cast<Square>(pop_lsb(attacks));
                moves.push_back(Move(sq, to, get_bit(their, to) ? MoveFlag::Capture : MoveFlag::Quiet));
            }
        }

        // King
        U64 king = board.get_pieces(Piece::KING, us);
        if (king) {
            Square sq = static_cast<Square>(pop_lsb(king));
            U64 attacks = king_attacks[static_cast<int>(sq)] & ~our;
            while (attacks) {
                Square to = static_cast<Square>(pop_lsb(attacks));
                moves.push_back(Move(sq, to, get_bit(their, to) ? MoveFlag::Capture : MoveFlag::Quiet));
            }
        }

        // Castling
        gen_castling_moves(board, moves, us);

        return moves;
    }

    // -----------------------------------------------------------------------
    // Legal move filter: make the move, check if king is in check, unmake
    std::vector<Move> MoveGen::generate_legal_moves(Board& board) {
        std::vector<Move> pseudo = generate_pseudo_legal_moves(board);
        std::vector<Move> legal;
        legal.reserve(pseudo.size());

        Color us   = board.get_side_to_move();
        Color them = (us == Color::WHITE) ? Color::BLACK : Color::WHITE;

        for (const Move& m : pseudo) {
            board.make_move(m);
            // Find the king's square after the move
            U64 king_bb = board.get_pieces(Piece::KING, us);
            if (king_bb) {
                Square king_sq = static_cast<Square>(lsb(king_bb));
                if (!is_square_attacked(board, king_sq, them)) {
                    legal.push_back(m);
                }
            }
            board.unmake_move(m);
        }

        return legal;
    }

    bool MoveGen::in_check(const Board& board) {
        Color us = board.get_side_to_move();
        U64 king_bb = board.get_pieces(Piece::KING, us);
        if (!king_bb) return false;
        Square king_sq = static_cast<Square>(lsb(king_bb));
        return is_square_attacked(board, king_sq, opposite(us));
    }

    std::vector<Move> MoveGen::generate_legal_noisy(Board& board) {
        std::vector<Move> pseudo = generate_pseudo_legal_moves(board);
        std::vector<Move> legal;
        Color us = board.get_side_to_move();
        Color them = opposite(us);
        for (const Move& m : pseudo) {
            if (!m.is_capture() && !m.is_promotion()) continue;
            board.make_move(m);
            U64 king_bb = board.get_pieces(Piece::KING, us);
            if (king_bb) {
                Square king_sq = static_cast<Square>(lsb(king_bb));
                if (!is_square_attacked(board, king_sq, them)) legal.push_back(m);
            }
            board.unmake_move(m);
        }
        return legal;
    }
}
