#include "eval.hpp"
#include "utils.hpp"

namespace engine {
    // Piece-square tables, white's perspective, a1 = index 0.
    static const int PST_PAWN[64] = {
         0,  0,  0,  0,  0,  0,  0,  0,
         5, 10, 10,-20,-20, 10, 10,  5,
         5, -5,-10,  0,  0,-10, -5,  5,
         0,  0,  0, 20, 20,  0,  0,  0,
         5,  5, 10, 25, 25, 10,  5,  5,
        10, 10, 20, 30, 30, 20, 10, 10,
        50, 50, 50, 50, 50, 50, 50, 50,
         0,  0,  0,  0,  0,  0,  0,  0
    };
    static const int PST_KNIGHT[64] = {
        -50,-40,-30,-30,-30,-30,-40,-50,
        -40,-20,  0,  5,  5,  0,-20,-40,
        -30,  5, 10, 15, 15, 10,  5,-30,
        -30,  0, 15, 20, 20, 15,  0,-30,
        -30,  5, 15, 20, 20, 15,  5,-30,
        -30,  0, 10, 15, 15, 10,  0,-30,
        -40,-20,  0,  0,  0,  0,-20,-40,
        -50,-40,-30,-30,-30,-30,-40,-50
    };
    static const int PST_BISHOP[64] = {
        -20,-10,-10,-10,-10,-10,-10,-20,
        -10,  5,  0,  0,  0,  0,  5,-10,
        -10, 10, 10, 10, 10, 10, 10,-10,
        -10,  0, 10, 10, 10, 10,  0,-10,
        -10,  5,  5, 10, 10,  5,  5,-10,
        -10,  0,  5, 10, 10,  5,  0,-10,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -20,-10,-10,-10,-10,-10,-10,-20
    };
    static const int PST_ROOK[64] = {
         0,  0,  0,  5,  5,  0,  0,  0,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
         5, 10, 10, 10, 10, 10, 10,  5,
         0,  0,  0,  0,  0,  0,  0,  0
    };
    static const int PST_QUEEN[64] = {
        -20,-10,-10, -5, -5,-10,-10,-20,
        -10,  0,  5,  0,  0,  0,  0,-10,
        -10,  5,  5,  5,  5,  5,  0,-10,
          0,  0,  5,  5,  5,  5,  0, -5,
         -5,  0,  5,  5,  5,  5,  0, -5,
        -10,  0,  5,  5,  5,  5,  0,-10,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -20,-10,-10, -5, -5,-10,-10,-20
    };
    static const int PST_KING_MG[64] = {
         20, 30, 10,  0,  0, 10, 30, 20,
         20, 20,  0,  0,  0,  0, 20, 20,
        -10,-20,-20,-20,-20,-20,-20,-10,
        -20,-30,-30,-40,-40,-30,-30,-20,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30
    };
    static const int PST_KING_EG[64] = {
        -50,-30,-30,-30,-30,-30,-30,-50,
        -30,-30,  0,  0,  0,  0,-30,-30,
        -30,-10, 20, 30, 30, 20,-10,-30,
        -30,-10, 30, 40, 40, 30,-10,-30,
        -30,-10, 30, 40, 40, 30,-10,-30,
        -30,-10, 20, 30, 30, 20,-10,-30,
        -30,-20,-10,  0,  0,-10,-20,-30,
        -50,-40,-30,-20,-20,-30,-40,-50
    };

    static const int MATERIAL[6] = {100, 320, 330, 500, 900, 0};

    static int pst_value(Piece p, Color c, int sq, bool endgame) {
        int idx = (c == Color::WHITE) ? sq : (sq ^ 56);
        if (p == Piece::KING) return endgame ? PST_KING_EG[idx] : PST_KING_MG[idx];
        static const int* tables[5] = {PST_PAWN, PST_KNIGHT, PST_BISHOP, PST_ROOK, PST_QUEEN};
        return tables[static_cast<int>(p)][idx];
    }

    static const U64 FILE_MASK[8] = {
        0x0101010101010101ULL, 0x0202020202020202ULL, 0x0404040404040404ULL, 0x0808080808080808ULL,
        0x1010101010101010ULL, 0x2020202020202020ULL, 0x4040404040404040ULL, 0x8080808080808080ULL
    };

    int Evaluator::evaluate(const Board& board) {
        int score = 0;
        int phase = 0;

        for (int i = 0; i < 6; i++) {
            Piece p = static_cast<Piece>(i);
            U64 ww = board.get_pieces(p, Color::WHITE);
            U64 bb = board.get_pieces(p, Color::BLACK);

            int count_w = popcount(ww);
            int count_b = popcount(bb);

            static const int PHASE_VAL[6] = {0, 1, 1, 2, 4, 0};
            phase += (count_w + count_b) * PHASE_VAL[i];

            score += count_w * MATERIAL[i];
            score -= count_b * MATERIAL[i];

            bool endgame = phase < 8;
            while (ww) {
                int sq = static_cast<int>(pop_lsb(ww));
                score += pst_value(p, Color::WHITE, sq, endgame);
            }
            while (bb) {
                int sq = static_cast<int>(pop_lsb(bb));
                score -= pst_value(p, Color::BLACK, sq, endgame);
            }
        }

        // Bishop pair
        if (popcount(board.get_pieces(Piece::BISHOP, Color::WHITE)) >= 2) score += 30;
        if (popcount(board.get_pieces(Piece::BISHOP, Color::BLACK)) >= 2) score -= 30;

        // Mobility proxy
        score += popcount(board.get_pieces(Piece::KNIGHT, Color::WHITE)) * 4;
        score += popcount(board.get_pieces(Piece::BISHOP, Color::WHITE)) * 5;
        score += popcount(board.get_pieces(Piece::ROOK,   Color::WHITE)) * 3;
        score -= popcount(board.get_pieces(Piece::KNIGHT, Color::BLACK)) * 4;
        score -= popcount(board.get_pieces(Piece::BISHOP, Color::BLACK)) * 5;
        score -= popcount(board.get_pieces(Piece::ROOK,   Color::BLACK)) * 3;

        // Pawn structure
        U64 wp = board.get_pieces(Piece::PAWN, Color::WHITE);
        U64 bp = board.get_pieces(Piece::PAWN, Color::BLACK);

        for (int f = 0; f < 8; f++) {
            U64 w_file = wp & FILE_MASK[f];
            U64 b_file = bp & FILE_MASK[f];
            int w_cnt = popcount(w_file);
            int b_cnt = popcount(b_file);

            if (w_cnt > 1) score -= 12 * (w_cnt - 1);
            if (b_cnt > 1) score += 12 * (b_cnt - 1);

            U64 adj = 0;
            if (f > 0) adj |= FILE_MASK[f - 1];
            if (f < 7) adj |= FILE_MASK[f + 1];
            if (w_cnt && !(wp & adj)) score -= 10;
            if (b_cnt && !(bp & adj)) score += 10;
        }

        // Passed pawns
        U64 w_pawns = wp;
        while (w_pawns) {
            int sq = static_cast<int>(pop_lsb(w_pawns));
            int rank = sq / 8;
            int file = sq % 8;
            U64 front = 0;
            for (int r = rank + 1; r < 8; r++) {
                front |= (1ULL << (r * 8 + file));
                if (file > 0) front |= (1ULL << (r * 8 + file - 1));
                if (file < 7) front |= (1ULL << (r * 8 + file + 1));
            }
            if ((bp & front) == 0) score += 10 + rank * 6;
        }
        U64 b_pawns = bp;
        while (b_pawns) {
            int sq = static_cast<int>(pop_lsb(b_pawns));
            int rank = sq / 8;
            int file = sq % 8;
            U64 front = 0;
            for (int r = rank - 1; r >= 0; r--) {
                front |= (1ULL << (r * 8 + file));
                if (file > 0) front |= (1ULL << (r * 8 + file - 1));
                if (file < 7) front |= (1ULL << (r * 8 + file + 1));
            }
            if ((wp & front) == 0) score -= 10 + (7 - rank) * 6;
        }

        // Rook on open / semi-open file
        U64 wr = board.get_pieces(Piece::ROOK, Color::WHITE);
        while (wr) {
            int sq = static_cast<int>(pop_lsb(wr));
            int file = sq % 8;
            if ((wp & FILE_MASK[file]) == 0) {
                score += 12;
                if ((bp & FILE_MASK[file]) == 0) score += 8;
            }
        }
        U64 br = board.get_pieces(Piece::ROOK, Color::BLACK);
        while (br) {
            int sq = static_cast<int>(pop_lsb(br));
            int file = sq % 8;
            if ((bp & FILE_MASK[file]) == 0) {
                score -= 12;
                if ((wp & FILE_MASK[file]) == 0) score -= 8;
            }
        }

        return (board.get_side_to_move() == Color::WHITE) ? score : -score;
    }
}
