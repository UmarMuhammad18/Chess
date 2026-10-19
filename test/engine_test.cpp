#include <gtest/gtest.h>
#include "../src/engine/attacks.hpp"
#include "../src/engine/board.hpp"
#include "../src/engine/movegen.hpp"
#include "../src/engine/notation.hpp"
#include "../src/engine/perft.hpp"
#include "../src/engine/utils.hpp"
#include "../src/engine/zobrist.hpp"
#include <cstdint>

using namespace engine;

namespace {
    // Standard perft positions from the Chess Programming Wiki
    constexpr const char* KIWIPETE = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
    constexpr const char* POS3     = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1";
    constexpr const char* POS4     = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
    constexpr const char* POS5     = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
}

class EngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        Zobrist::init();
        MoveGen::init();
    }

    static uint64_t perft_fen(const char* fen, int depth) {
        Board b;
        EXPECT_TRUE(b.load_fen(fen));
        return perft(b, depth);
    }
};

static bool has_move(const MoveList& moves, Square from, Square to, MoveFlag flag) {
    for (const auto& m : moves) {
        if (m.get_from() == from && m.get_to() == to && m.get_flag() == flag) return true;
    }
    return false;
}

TEST_F(EngineTest, FENParsingStartPos) {
    Board b;
    ASSERT_TRUE(b.load_fen(START_FEN));
    EXPECT_EQ(popcount(b.get_pieces(Piece::PAWN,   Color::WHITE)), 8);
    EXPECT_EQ(popcount(b.get_pieces(Piece::PAWN,   Color::BLACK)), 8);
    EXPECT_EQ(popcount(b.get_pieces(Piece::KING,   Color::WHITE)), 1);
    EXPECT_EQ(popcount(b.get_pieces(Piece::KING,   Color::BLACK)), 1);
    EXPECT_EQ(popcount(b.get_pieces(Color::WHITE)), 16);
    EXPECT_EQ(popcount(b.get_pieces(Color::BLACK)), 16);
    EXPECT_TRUE(get_bit(b.get_pieces(Piece::KING, Color::WHITE), Square::E1));
    EXPECT_TRUE(get_bit(b.get_pieces(Piece::KING, Color::BLACK), Square::E8));
}

TEST_F(EngineTest, FenRoundTrip) {
    for (const char* fen : {START_FEN, KIWIPETE, POS3, POS4, POS5,
                            "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1"}) {
        Board b;
        ASSERT_TRUE(b.load_fen(fen));
        EXPECT_EQ(b.get_fen(), fen);
    }
}

TEST_F(EngineTest, InvalidFenIsRejectedAndBoardUnchanged) {
    Board b;
    ASSERT_TRUE(b.load_fen(START_FEN));
    EXPECT_FALSE(b.load_fen("not a fen"));
    EXPECT_FALSE(b.load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQ1BNR w KQkq - 0 1")); // no white king
    EXPECT_FALSE(b.load_fen("rnbqkbnr/pppppppp/9/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));
    EXPECT_FALSE(b.load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR x KQkq - 0 1"));
    EXPECT_EQ(b.get_fen(), START_FEN);
}

TEST_F(EngineTest, MagicAttacksMatchSlowRays) {
    uint64_t seed = 12345;
    auto next = [&] { seed ^= seed << 13; seed ^= seed >> 7; seed ^= seed << 17; return seed; };
    for (int sq = 0; sq < 64; sq++) {
        for (int i = 0; i < 200; i++) {
            U64 occ = next() & next();
            Square s = static_cast<Square>(sq);
            ASSERT_EQ(attacks::bishop(s, occ), attacks::bishop_slow(s, occ));
            ASSERT_EQ(attacks::rook(s, occ), attacks::rook_slow(s, occ));
        }
    }
}

TEST_F(EngineTest, StartingPositionLegalMoves) {
    Board b;
    b.load_fen(START_FEN);
    auto moves = MoveGen::generate_legal_moves(b);
    EXPECT_EQ(moves.size(), 20);
}

TEST_F(EngineTest, MakeMoveAndUnmake) {
    Board b;
    b.load_fen(START_FEN);
    U64 original_hash = b.hash_key;
    Move m(Square::E2, Square::E4, MoveFlag::DoublePawnPush);
    b.make_move(m);
    EXPECT_NE(b.hash_key, original_hash);
    EXPECT_EQ(b.get_side_to_move(), Color::BLACK);
    b.unmake_move(m);
    EXPECT_EQ(b.hash_key, original_hash);
    EXPECT_EQ(b.get_side_to_move(), Color::WHITE);
}

TEST_F(EngineTest, NullMoveRestoresState) {
    Board b;
    ASSERT_TRUE(b.load_fen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1"));
    std::string fen = b.get_fen();
    U64 hash = b.hash_key;
    b.make_null_move();
    EXPECT_EQ(b.get_side_to_move(), Color::WHITE);
    EXPECT_EQ(b.get_en_passant_sq(), Square::NONE);
    EXPECT_NE(b.hash_key, hash);
    b.unmake_null_move();
    EXPECT_EQ(b.hash_key, hash);
    EXPECT_EQ(b.get_fen(), fen);
}

TEST_F(EngineTest, PerftStartPos) {
    EXPECT_EQ(perft_fen(START_FEN, 1), 20u);
    EXPECT_EQ(perft_fen(START_FEN, 2), 400u);
    EXPECT_EQ(perft_fen(START_FEN, 3), 8902u);
    EXPECT_EQ(perft_fen(START_FEN, 4), 197281u);
}

TEST_F(EngineTest, PerftKiwipete) {
    EXPECT_EQ(perft_fen(KIWIPETE, 1), 48u);
    EXPECT_EQ(perft_fen(KIWIPETE, 2), 2039u);
    EXPECT_EQ(perft_fen(KIWIPETE, 3), 97862u);
}

TEST_F(EngineTest, PerftPosition3) {
    EXPECT_EQ(perft_fen(POS3, 1), 14u);
    EXPECT_EQ(perft_fen(POS3, 2), 191u);
    EXPECT_EQ(perft_fen(POS3, 3), 2812u);
    EXPECT_EQ(perft_fen(POS3, 4), 43238u);
}

TEST_F(EngineTest, PerftPosition4) {
    EXPECT_EQ(perft_fen(POS4, 1), 6u);
    EXPECT_EQ(perft_fen(POS4, 2), 264u);
    EXPECT_EQ(perft_fen(POS4, 3), 9467u);
}

TEST_F(EngineTest, PerftPosition5) {
    EXPECT_EQ(perft_fen(POS5, 1), 44u);
    EXPECT_EQ(perft_fen(POS5, 2), 1486u);
    EXPECT_EQ(perft_fen(POS5, 3), 62379u);
}

// Deep perft: millions of nodes, run with the "slow" ctest label
TEST_F(EngineTest, SlowPerftDeep) {
    EXPECT_EQ(perft_fen(START_FEN, 5), 4865609u);
    EXPECT_EQ(perft_fen(KIWIPETE, 4), 4085603u);
    EXPECT_EQ(perft_fen(POS3, 5), 674624u);
    EXPECT_EQ(perft_fen(POS4, 4), 422333u);
    EXPECT_EQ(perft_fen(POS5, 4), 2103487u);
}

TEST_F(EngineTest, ZobristMatchesLoadedFen) {
    Board played;
    played.load_fen(START_FEN);
    played.make_move(Move(Square::E2, Square::E4, MoveFlag::DoublePawnPush));

    Board loaded;
    loaded.load_fen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
    EXPECT_EQ(played.hash_key, loaded.hash_key);
}

TEST_F(EngineTest, PromotionCaptureRemovesVictim) {
    Board b;
    ASSERT_TRUE(b.load_fen("k7/8/8/8/8/8/p7/1N2K3 b - - 0 1"));
    U64 hash_before = b.hash_key;
    Move promo(Square::A2, Square::B1, MoveFlag::PromoQueenCap);
    b.make_move(promo);

    EXPECT_EQ(b.piece_on(Square::A2), Piece::NONE);
    EXPECT_EQ(b.piece_on(Square::B1), Piece::QUEEN);
    EXPECT_EQ(b.color_on(Square::B1), Color::BLACK);
    EXPECT_EQ(popcount(b.get_pieces(Piece::KNIGHT, Color::WHITE)), 0);
    EXPECT_EQ(popcount(b.get_pieces(Piece::PAWN, Color::BLACK)), 0);

    b.unmake_move(promo);
    EXPECT_EQ(b.hash_key, hash_before);
    EXPECT_EQ(b.piece_on(Square::A2), Piece::PAWN);
    EXPECT_EQ(b.piece_on(Square::B1), Piece::KNIGHT);
}

TEST_F(EngineTest, PawnPushToSeventhIsNotPromotion) {
    Board b;
    ASSERT_TRUE(b.load_fen("4k3/8/4P3/8/8/8/8/4K3 w - - 0 1"));
    auto moves = MoveGen::generate_legal_moves(b);
    EXPECT_TRUE(has_move(moves, Square::E6, Square::E7, MoveFlag::Quiet));
    EXPECT_FALSE(has_move(moves, Square::E6, Square::E7, MoveFlag::PromoQueen));
}

TEST_F(EngineTest, BlackCannotCastleThroughCheck) {
    Board b;
    ASSERT_TRUE(b.load_fen("r3k2r/8/8/8/8/8/8/5R1K b kq - 0 1"));
    auto moves = MoveGen::generate_legal_moves(b);
    EXPECT_FALSE(has_move(moves, Square::E8, Square::G8, MoveFlag::KingCastle));
    EXPECT_TRUE(has_move(moves, Square::E8, Square::C8, MoveFlag::QueenCastle));
}

TEST_F(EngineTest, BlackCannotCastleOutOfCheck) {
    Board b;
    ASSERT_TRUE(b.load_fen("r3k2r/8/8/8/8/8/8/4R2K b kq - 0 1"));
    auto moves = MoveGen::generate_legal_moves(b);
    EXPECT_FALSE(has_move(moves, Square::E8, Square::G8, MoveFlag::KingCastle));
    EXPECT_FALSE(has_move(moves, Square::E8, Square::C8, MoveFlag::QueenCastle));
}

TEST_F(EngineTest, StalemateIsNotCheckmate) {
    Board stalemate;
    ASSERT_TRUE(stalemate.load_fen("k7/8/1Q6/8/8/8/8/K7 b - - 0 1"));
    auto stale_moves = MoveGen::generate_legal_moves(stalemate);
    EXPECT_TRUE(stale_moves.empty());
    EXPECT_FALSE(MoveGen::is_square_attacked(stalemate, Square::A8, Color::WHITE));

    Board mate;
    ASSERT_TRUE(mate.load_fen("k7/1Q6/1K6/8/8/8/8/8 b - - 0 1"));
    auto mate_moves = MoveGen::generate_legal_moves(mate);
    EXPECT_TRUE(mate_moves.empty());
    EXPECT_TRUE(MoveGen::is_square_attacked(mate, Square::A8, Color::WHITE));
}

TEST_F(EngineTest, ThreefoldRepetitionCount) {
    Board b;
    ASSERT_TRUE(b.load_fen(START_FEN));
    const char* shuffle[] = {"g1f3", "g8f6", "f3g1", "f6g8"};
    EXPECT_EQ(b.repetition_count(), 0);
    for (int round = 1; round <= 2; round++) {
        for (const char* text : shuffle) {
            Move m = parse_uci_move(b, text);
            ASSERT_FALSE(m.is_null()) << text;
            b.make_move(m);
        }
        EXPECT_EQ(b.repetition_count(), round);
    }
}

TEST_F(EngineTest, FiftyMoveAndInsufficientMaterial) {
    Board b;
    ASSERT_TRUE(b.load_fen("8/8/4k3/8/8/4K3/8/8 w - - 100 80"));
    EXPECT_TRUE(b.is_fifty_move_draw());
    EXPECT_TRUE(b.is_insufficient_material());
    ASSERT_TRUE(b.load_fen("8/8/4k3/8/8/4KN2/8/8 w - - 0 1"));
    EXPECT_TRUE(b.is_insufficient_material());
    ASSERT_TRUE(b.load_fen("8/8/4k3/8/8/4KNN1/8/8 w - - 0 1"));
    EXPECT_FALSE(b.is_insufficient_material());
    ASSERT_TRUE(b.load_fen("8/8/4k3/8/8/4K3/4P3/8 w - - 0 1"));
    EXPECT_FALSE(b.is_insufficient_material());
}

TEST_F(EngineTest, UciNotationRoundTrip) {
    Board b;
    ASSERT_TRUE(b.load_fen(POS4));
    for (const Move& m : MoveGen::generate_legal_moves(b)) {
        EXPECT_EQ(parse_uci_move(b, move_to_uci(m)), m);
    }
    EXPECT_TRUE(parse_uci_move(b, "e2e4").is_null());
}

TEST_F(EngineTest, SanNotation) {
    Board b;
    ASSERT_TRUE(b.load_fen(START_FEN));
    EXPECT_EQ(move_to_san(b, parse_uci_move(b, "g1f3")), "Nf3");
    EXPECT_EQ(move_to_san(b, parse_uci_move(b, "e2e4")), "e4");

    // Knights on b1 and f3 can both reach d2: needs file disambiguation
    ASSERT_TRUE(b.load_fen("4k3/8/8/8/8/5N2/8/1N2K3 w - - 0 1"));
    EXPECT_EQ(move_to_san(b, parse_uci_move(b, "b1d2")), "Nbd2");

    ASSERT_TRUE(b.load_fen("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1"));
    EXPECT_EQ(move_to_san(b, parse_uci_move(b, "e1g1")), "O-O");
    EXPECT_EQ(move_to_san(b, parse_uci_move(b, "e1c1")), "O-O-O");

    ASSERT_TRUE(b.load_fen("k7/2P5/1K6/8/8/8/8/8 w - - 0 1"));
    EXPECT_EQ(move_to_san(b, parse_uci_move(b, "c7c8q")), "c8=Q#");
    EXPECT_EQ(move_to_san(b, parse_uci_move(b, "c7c8n")), "c8=N");

    ASSERT_TRUE(b.load_fen("k7/8/KQ6/8/8/8/8/8 w - - 0 1"));
    EXPECT_EQ(move_to_san(b, parse_uci_move(b, "b6b7")), "Qb7#");
}
