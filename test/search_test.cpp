#include <gtest/gtest.h>
#include "../src/engine/eval.hpp"
#include "../src/engine/movegen.hpp"
#include "../src/engine/notation.hpp"
#include "../src/engine/search.hpp"

using namespace engine;

class SearchTest : public ::testing::Test {
protected:
    void SetUp() override { MoveGen::init(); }

    static std::string best_uci(const char* fen, int depth) {
        Board b;
        EXPECT_TRUE(b.load_fen(fen));
        Search search(4);
        return move_to_uci(search.search_best_move(b, depth));
    }
};

TEST_F(SearchTest, PrefersMateOverStalemate) {
    Board b;
    ASSERT_TRUE(b.load_fen("k7/8/KQ6/8/8/8/8/8 w - - 0 1"));
    Search search(4);
    Move best = search.search_best_move(b, 2);
    EXPECT_EQ(best.get_from(), Square::B6);
    EXPECT_TRUE(best.get_to() == Square::B7 || best.get_to() == Square::A7);
}

TEST_F(SearchTest, ReportsMateScore) {
    Board b;
    ASSERT_TRUE(b.load_fen("k7/8/KQ6/8/8/8/8/8 w - - 0 1"));
    Search search(4);
    search.search_best_move(b, 4);
    EXPECT_TRUE(search.last_info().is_mate());
    EXPECT_EQ(search.last_info().mate_in(), 1);
}

TEST_F(SearchTest, FindsBackRankMate) {
    EXPECT_EQ(best_uci("6k1/5ppp/8/8/8/8/3Q1PPP/3R2K1 w - - 0 1", 4), "d2d8");
}

TEST_F(SearchTest, WinsHangingQueen) {
    EXPECT_EQ(best_uci("rnb1kbnr/pppp1ppp/8/4p1q1/3P4/2N5/PPP1PPPP/R1BQKBNR w KQkq - 0 1", 4), "c1g5");
}

TEST_F(SearchTest, QuiescenceAvoidsPoisonedCapture) {
    // Qxb7?? loses the queen to ...Rxb7 one ply past the horizon of a 1-ply search
    std::string move = best_uci("1r4k1/1p3ppp/8/8/8/8/5PPP/1Q4K1 w - - 0 1", 1);
    EXPECT_NE(move, "b1b7");
}

TEST_F(SearchTest, ReturnsNullMoveWhenNoLegalMoves) {
    Board b;
    ASSERT_TRUE(b.load_fen("k7/1Q6/1K6/8/8/8/8/8 b - - 0 1"));
    Search search(4);
    EXPECT_TRUE(search.search_best_move(b, 3).is_null());
}

TEST_F(SearchTest, ClaimsRepetitionDrawWhenLosing) {
    // Black is a queen down; repeating the position is its best outcome. After the
    // shuffles below, ...Kh8 repeats a position already seen, which scores 0.
    Board b;
    ASSERT_TRUE(b.load_fen("7k/8/8/8/8/8/6Q1/K7 b - - 0 1"));
    for (const char* text : {"h8h7", "g2g3", "h7h8", "g3g2", "h8h7", "g2g3"}) {
        Move m = parse_uci_move(b, text);
        ASSERT_FALSE(m.is_null()) << text;
        b.make_move(m);
    }
    EXPECT_EQ(b.repetition_count(), 1);

    Search search(4);
    EXPECT_EQ(move_to_uci(search.search_best_move(b, 4)), "h7h8");
    EXPECT_EQ(search.last_info().score, 0);
}

TEST_F(SearchTest, MovetimeIsRespected) {
    Board b;
    ASSERT_TRUE(b.load_fen(START_FEN));
    Search search(4);
    SearchLimits limits;
    limits.movetime = 200;
    auto start = std::chrono::steady_clock::now();
    Move m = search.think(b, limits);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
    EXPECT_FALSE(m.is_null());
    EXPECT_LT(ms, 600);
    EXPECT_GE(search.last_info().depth, 3);
}

TEST_F(SearchTest, PvIsLegalSequence) {
    Board b;
    ASSERT_TRUE(b.load_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"));
    Search search(4);
    search.search_best_move(b, 5);
    const auto& pv = search.last_info().pv;
    ASSERT_FALSE(pv.empty());
    Board copy = b;
    for (Move m : pv) {
        ASSERT_FALSE(parse_uci_move(copy, move_to_uci(m)).is_null()) << move_to_uci(m);
        copy.make_move(m);
    }
}

TEST_F(SearchTest, EvaluationIsSymmetric) {
    // Mirroring the position and swapping colours must give the same side-to-move score
    Board w, b;
    ASSERT_TRUE(w.load_fen("r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3"));
    ASSERT_TRUE(b.load_fen("rnbqkb1r/pppp1ppp/5n2/4p3/4P3/2N5/PPPP1PPP/R1BQKBNR b KQkq - 2 3"));
    EXPECT_EQ(Evaluator::evaluate(w), Evaluator::evaluate(b));
}
