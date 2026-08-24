#pragma once
#include <cstdint>

namespace engine {
    using U64 = uint64_t;

    enum class Color { WHITE, BLACK, BOTH };
    enum class Piece { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, NONE };

    inline Color opposite(Color c) {
        return c == Color::WHITE ? Color::BLACK : Color::WHITE;
    }

    enum class GameResult {
        Ongoing,
        WhiteMates,
        BlackMates,
        Stalemate,
        DrawFifty,
        DrawRepetition,
        DrawMaterial,
        WhiteResigns,
        BlackResigns
    };

    enum class Square : int {
        A1, B1, C1, D1, E1, F1, G1, H1,
        A2, B2, C2, D2, E2, F2, G2, H2,
        A3, B3, C3, D3, E3, F3, G3, H3,
        A4, B4, C4, D4, E4, F4, G4, H4,
        A5, B5, C5, D5, E5, F5, G5, H5,
        A6, B6, C6, D6, E6, F6, G6, H6,
        A7, B7, C7, D7, E7, F7, G7, H7,
        A8, B8, C8, D8, E8, F8, G8, H8,
        NONE = 64
    };

    enum class File { FileA, FileB, FileC, FileD, FileE, FileF, FileG, FileH };
    enum class Rank { Rank1, Rank2, Rank3, Rank4, Rank5, Rank6, Rank7, Rank8 };

    enum Castling { WK = 1, WQ = 2, BK = 4, BQ = 8 };

    // 16-bit Move layout:
    // bits  0-5 : from square
    // bits  6-11: to square
    // bits 12-15: flag (MoveFlag)
    enum class MoveFlag : uint8_t {
        Quiet             = 0,
        DoublePawnPush    = 1,
        KingCastle        = 2,
        QueenCastle       = 3,
        Capture           = 4,
        EnPassant         = 5,
        PromoKnight       = 8,
        PromoBishop       = 9,
        PromoRook         = 10,
        PromoQueen        = 11,
        PromoKnightCap    = 12,
        PromoBishopCap    = 13,
        PromoRookCap      = 14,
        PromoQueenCap     = 15,
    };

    struct Move {
        uint16_t data{0};

        Move() = default;
        explicit Move(uint16_t d) : data(d) {}

        Move(Square from, Square to, MoveFlag flag = MoveFlag::Quiet) {
            data  = (static_cast<uint16_t>(from)  & 0x3F);
            data |= (static_cast<uint16_t>(to)    & 0x3F) << 6;
            data |= (static_cast<uint16_t>(flag)  & 0x0F) << 12;
        }

        Square   get_from() const { return static_cast<Square>( data        & 0x3F); }
        Square   get_to()   const { return static_cast<Square>((data >> 6)  & 0x3F); }
        MoveFlag get_flag() const { return static_cast<MoveFlag>((data >> 12) & 0x0F); }

        bool is_capture()   const {
            auto f = get_flag();
            return f == MoveFlag::Capture    || f == MoveFlag::EnPassant      ||
                   f == MoveFlag::PromoKnightCap || f == MoveFlag::PromoBishopCap ||
                   f == MoveFlag::PromoRookCap   || f == MoveFlag::PromoQueenCap;
        }
        bool is_promotion() const {
            auto f = get_flag();
            return f == MoveFlag::PromoKnight || f == MoveFlag::PromoBishop ||
                   f == MoveFlag::PromoRook   || f == MoveFlag::PromoQueen  ||
                   f == MoveFlag::PromoKnightCap || f == MoveFlag::PromoBishopCap ||
                   f == MoveFlag::PromoRookCap   || f == MoveFlag::PromoQueenCap;
        }
        Piece promo_piece() const {
            auto f = get_flag();
            if (f == MoveFlag::PromoKnight || f == MoveFlag::PromoKnightCap) return Piece::KNIGHT;
            if (f == MoveFlag::PromoBishop || f == MoveFlag::PromoBishopCap) return Piece::BISHOP;
            if (f == MoveFlag::PromoRook   || f == MoveFlag::PromoRookCap)   return Piece::ROOK;
            if (f == MoveFlag::PromoQueen  || f == MoveFlag::PromoQueenCap)  return Piece::QUEEN;
            return Piece::NONE;
        }
    };
}
