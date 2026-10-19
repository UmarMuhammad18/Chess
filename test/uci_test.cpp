#include <gtest/gtest.h>
#include "../src/uci/uci.hpp"
#include <sstream>

TEST(UciTest, Handshake) {
    std::ostringstream out;
    uci::UciEngine engine(out);
    engine.handle("uci");
    engine.handle("isready");
    std::string text = out.str();
    EXPECT_NE(text.find("id name"), std::string::npos);
    EXPECT_NE(text.find("uciok"), std::string::npos);
    EXPECT_NE(text.find("readyok"), std::string::npos);
}

TEST(UciTest, PositionWithMovesThenGo) {
    std::ostringstream out;
    uci::UciEngine engine(out);
    engine.handle("position startpos moves e2e4 e7e5 g1f3");
    engine.handle("d");
    EXPECT_NE(out.str().find("rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2"), std::string::npos);

    engine.handle("go depth 4");
    engine.wait();
    std::string text = out.str();
    EXPECT_NE(text.find("info depth 4"), std::string::npos);
    EXPECT_NE(text.find("bestmove "), std::string::npos);
    EXPECT_EQ(text.find("bestmove 0000"), std::string::npos);
}

TEST(UciTest, FindsMateFromFen) {
    std::ostringstream out;
    uci::UciEngine engine(out);
    engine.handle("position fen k7/8/KQ6/8/8/8/8/8 w - - 0 1");
    engine.handle("go depth 3");
    engine.wait();
    std::string text = out.str();
    EXPECT_NE(text.find("score mate 1"), std::string::npos);
    EXPECT_TRUE(text.find("bestmove b6b7") != std::string::npos || text.find("bestmove b6a7") != std::string::npos);
}

TEST(UciTest, StopInterruptsInfiniteSearch) {
    std::ostringstream out;
    uci::UciEngine engine(out);
    engine.handle("position startpos");
    engine.handle("go infinite");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    engine.handle("stop");
    EXPECT_NE(out.str().find("bestmove "), std::string::npos);
}

TEST(UciTest, IllegalMoveIsReported) {
    std::ostringstream out;
    uci::UciEngine engine(out);
    engine.handle("position startpos moves e2e5");
    EXPECT_NE(out.str().find("illegal move: e2e5"), std::string::npos);
}
