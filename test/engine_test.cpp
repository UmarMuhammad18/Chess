#include <gtest/gtest.h>
#include "../src/engine/board.hpp"
#include "../src/engine/eval.hpp"
#include "../src/engine/movegen.hpp"
#include "../src/engine/notation.hpp"
#include "../src/engine/search.hpp"
#include "../src/engine/utils.hpp"
#include "../src/engine/zobrist.hpp"
#include <cstdint>
#include <string>
#include <vector>

using namespace engine;

class EngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        Zobrist::init();
        MoveGen::init();
    }
};

static uint64_t perft(Board& board, int depth) {
    auto moves = MoveGen::generate_legal_moves(board);
    if (depth <= 1) return moves.size();
    uint64_t nodes = 0;
    for (const auto& m : moves) {
        board.make_move(m);
        nodes += perft(board, depth - 1);
        board.unmake_move(m);
    }
    return nodes;
}

static bool has_move(const std::vector<Move>& moves, Square from, Square to, MoveFlag flag) {
    for (const auto& m : moves) {
        if (m.get_from() == from && m.get_to() == to && m.get_flag() == flag) return true;
    }
    return false;
}

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

TEST_F(EngineTest, PerftStartPos) {
    Board b;
    b.load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    EXPECT_EQ(perft(b, 1), 20u);
    EXPECT_EQ(perft(b, 2), 400u);
    EXPECT_EQ(perft(b, 3), 8902u);
    EXPECT_EQ(perft(b, 4), 197281u);
}

TEST_F(EngineTest, PerftKiwipete) {
    Board b;
    b.load_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    EXPECT_EQ(perft(b, 1), 48u);
    EXPECT_EQ(perft(b, 2), 2039u);
    EXPECT_EQ(perft(b, 3), 97862u);
}

TEST_F(EngineTest, ZobristMatchesLoadedFen) {
    Board played;
    played.load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
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

TEST_F(EngineTest, SearchPrefersMateOverStalemate) {
    Board b;
    ASSERT_TRUE(b.load_fen("k7/8/KQ6/8/8/8/8/8 w - - 0 1"));
    Search search;
    Move best = search.search_best_move(b, 2);
    EXPECT_EQ(best.get_from(), Square::B6);
    std::string san = move_to_san(b, best);
    EXPECT_NE(san.find('#'), std::string::npos) << san;
}

TEST_F(EngineTest, PieceSquareTablesPreferCenterPawn) {
    Board e4, a3;
    ASSERT_TRUE(e4.load_fen("4k3/8/8/8/4P3/8/8/4K3 w - - 0 1"));
    ASSERT_TRUE(a3.load_fen("4k3/8/8/8/8/P7/8/4K3 w - - 0 1"));
    EXPECT_GT(Evaluator::evaluate(e4), Evaluator::evaluate(a3));
}

TEST_F(EngineTest, PieceSquareTablesPreferAdvancedPawn) {
    Board e7, e2;
    ASSERT_TRUE(e7.load_fen("4k3/4P3/8/8/8/8/8/4K3 w - - 0 1"));
    ASSERT_TRUE(e2.load_fen("4k3/8/8/8/8/8/4P3/4K3 w - - 0 1"));
    EXPECT_GT(Evaluator::evaluate(e7), Evaluator::evaluate(e2));
}

TEST_F(EngineTest, UciAndSanForE2E4) {
    Board b;
    b.load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    Move m = move_from_uci(b, "e2e4");
    EXPECT_EQ(m.get_from(), Square::E2);
    EXPECT_EQ(m.get_to(), Square::E4);
    EXPECT_EQ(move_to_uci(m), "e2e4");
    EXPECT_EQ(move_to_san(b, m), "e4");
}

TEST_F(EngineTest, CastlingSan) {
    Board b;
    ASSERT_TRUE(b.load_fen("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1"));
    Move m = move_from_uci(b, "e1g1");
    EXPECT_EQ(move_to_san(b, m), "O-O");
}

TEST_F(EngineTest, ThreefoldRepetitionIsDraw) {
    Board b;
    b.load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    auto play = [&](const char* u) {
        Move m = move_from_uci(b, u);
        ASSERT_NE(m.data, 0u);
        b.make_move(m);
    };
    play("g1f3"); play("g8f6");
    play("f3g1"); play("f6g8");
    play("g1f3"); play("g8f6");
    play("f3g1"); play("f6g8");
    EXPECT_GE(b.repetition_count(), 3);
    EXPECT_EQ(position_result(b), GameResult::DrawRepetition);
}

TEST_F(EngineTest, CheckmateResult) {
    Board b;
    ASSERT_TRUE(b.load_fen("k7/1Q6/1K6/8/8/8/8/8 b - - 0 1"));
    EXPECT_EQ(position_result(b), GameResult::WhiteMates);
}

TEST_F(EngineTest, StalemateResult) {
    Board b;
    ASSERT_TRUE(b.load_fen("k7/8/1Q6/8/8/8/8/K7 b - - 0 1"));
    EXPECT_EQ(position_result(b), GameResult::Stalemate);
}

TEST_F(EngineTest, TimedSearchReturnsLegalMove) {
    Board b;
    b.load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    Search search;
    SearchLimits lim;
    lim.movetime_ms = 80;
    lim.max_depth = 8;
    lim.use_clock = true;
    SearchResult r = search.search(b, lim);
    EXPECT_NE(r.best.data, 0u);
    EXPECT_GE(r.depth, 1);
    auto legal = MoveGen::generate_legal_moves(b);
    bool found = false;
    for (const auto& m : legal) {
        if (m.data == r.best.data) found = true;
    }
    EXPECT_TRUE(found);
}

TEST_F(EngineTest, PromotionUci) {
    Board b;
    ASSERT_TRUE(b.load_fen("4k3/P7/8/8/8/8/8/4K3 w - - 0 1"));
    Move m = move_from_uci(b, "a7a8q");
    EXPECT_TRUE(m.is_promotion());
    EXPECT_EQ(m.promo_piece(), Piece::QUEEN);
    EXPECT_EQ(move_to_uci(m), "a7a8q");
    EXPECT_EQ(move_to_san(b, m), "a8=Q+");
}
