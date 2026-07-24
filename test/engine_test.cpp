#include <gtest/gtest.h>
#include "../src/engine/board.hpp"
#include "../src/engine/movegen.hpp"
#include "../src/engine/utils.hpp"
#include "../src/engine/zobrist.hpp"

using namespace engine;

class EngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        Zobrist::init();
        MoveGen::init();
    }
};

TEST_F(EngineTest, FENParsingStartPos) {
    Board b;
    ASSERT_TRUE(b.load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));
    EXPECT_EQ(popcount(b.get_pieces(Piece::PAWN,   Color::WHITE)), 8);
    EXPECT_EQ(popcount(b.get_pieces(Piece::PAWN,   Color::BLACK)), 8);
    EXPECT_EQ(popcount(b.get_pieces(Piece::KING,   Color::WHITE)), 1);
    EXPECT_EQ(popcount(b.get_pieces(Piece::KING,   Color::BLACK)), 1);
    EXPECT_EQ(popcount(b.get_pieces(Color::WHITE)), 16);
    EXPECT_EQ(popcount(b.get_pieces(Color::BLACK)), 16);
    EXPECT_TRUE(get_bit(b.get_pieces(Piece::KING, Color::WHITE), Square::E1));
    EXPECT_TRUE(get_bit(b.get_pieces(Piece::KING, Color::BLACK), Square::E8));
}

TEST_F(EngineTest, StartingPositionLegalMoves) {
    Board b;
    b.load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    auto moves = MoveGen::generate_legal_moves(b);
    // Starting position: 16 pawn moves + 4 knight moves = 20
    EXPECT_EQ(moves.size(), 20u);
}

TEST_F(EngineTest, MakeMoveAndUnmake) {
    Board b;
    b.load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    U64 original_hash = b.hash_key;
    Move m(Square::E2, Square::E4, MoveFlag::DoublePawnPush);
    b.make_move(m);
    EXPECT_NE(b.hash_key, original_hash);
    EXPECT_EQ(b.get_side_to_move(), Color::BLACK);
    b.unmake_move(m);
    EXPECT_EQ(b.hash_key, original_hash);
    EXPECT_EQ(b.get_side_to_move(), Color::WHITE);
}
