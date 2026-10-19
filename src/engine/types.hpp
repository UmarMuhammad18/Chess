#pragma once
#include <cstdint>

namespace engine {
    using U64 = uint64_t;

    enum class Color { WHITE, BLACK, BOTH };
    enum class Piece { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, NONE };

    constexpr Color opposite(Color c) { return c == Color::WHITE ? Color::BLACK : Color::WHITE; }

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

    // Search limits and score scale (centipawns)
    constexpr int MAX_PLY    = 128;
    constexpr int MAX_MOVES  = 256;
    constexpr int INF_SCORE  = 32000;
    constexpr int MATE_SCORE = 31000;
    constexpr int MATE_BOUND = MATE_SCORE - MAX_PLY; // |score| >= this means "mate found"

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

        bool is_null() const { return data == 0; }
        bool operator==(const Move& o) const { return data == o.data; }
        bool operator!=(const Move& o) const { return data != o.data; }

        // Flags 4-5 and 12-15 have bit 2 set: Capture, EnPassant and the promo-captures
        bool is_capture()   const { return (static_cast<int>(get_flag()) & 4) != 0; }
        bool is_promotion() const { return (static_cast<int>(get_flag()) & 8) != 0; }
        bool is_castle()    const { return get_flag() == MoveFlag::KingCastle || get_flag() == MoveFlag::QueenCastle; }
        Piece promo_piece() const {
            if (!is_promotion()) return Piece::NONE;
            // Low two bits of the flag select N, B, R, Q
            return static_cast<Piece>(static_cast<int>(Piece::KNIGHT) + (static_cast<int>(get_flag()) & 3));
        }
    };

    // Fixed-capacity move container; avoids heap allocation in move generation
    struct MoveList {
        Move moves[MAX_MOVES];
        int  count{0};

        void push_back(Move m) { moves[count++] = m; }
        int  size()  const { return count; }
        bool empty() const { return count == 0; }
        void clear() { count = 0; }

        Move&       operator[](int i)       { return moves[i]; }
        const Move& operator[](int i) const { return moves[i]; }
        Move*       begin()       { return moves; }
        Move*       end()         { return moves + count; }
        const Move* begin() const { return moves; }
        const Move* end()   const { return moves + count; }
    };
}
