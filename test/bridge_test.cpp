#include <gtest/gtest.h>
#include "chess_c_api.h"
#include <string>

TEST(BridgeTest, PlaysAndDetectsMate) {
    ChessEngine* e = chess_create();
    ASSERT_NE(e, nullptr);
    EXPECT_EQ(chess_side_to_move(e), 0);

    char buf[2048];
    EXPECT_EQ(chess_legal_moves(e, buf, sizeof(buf)), 20);
    EXPECT_EQ(chess_make_uci(e, "e2e5"), 0);
    EXPECT_EQ(chess_make_uci(e, "e2e4"), 1);
    EXPECT_EQ(chess_side_to_move(e), 1);

    ASSERT_EQ(chess_load_fen(e, "k7/8/KQ6/8/8/8/8/8 w - - 0 1"), 1);
    char best[8];
    chess_search(e, 100, best, sizeof(best));
    std::string move = best;
    EXPECT_TRUE(move == "b6b7" || move == "b6a7") << move;
    ASSERT_EQ(chess_make_uci(e, best), 1);
    EXPECT_EQ(chess_result(e), 1);

    EXPECT_EQ(chess_load_fen(e, "garbage"), 0);
    chess_destroy(e);
}
