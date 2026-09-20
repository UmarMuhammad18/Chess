#include "chess_c_api.h"
#include "../engine/board.hpp"
#include "../engine/movegen.hpp"
#include "../engine/notation.hpp"
#include "../engine/search.hpp"
#include "../engine/zobrist.hpp"
#include <cstdio>
#include <limits>
#include <string>

struct ChessEngine {
    engine::Board board;
    engine::Search search;
    bool inited{false};
};

static void ensure_init(ChessEngine* e) {
    if (!e || e->inited) return;
    engine::MoveGen::init();
    engine::Zobrist::init();
    e->inited = true;
}

ChessEngine* chess_create(void) {
    auto* e = new ChessEngine();
    ensure_init(e);
    e->board.load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    return e;
}

void chess_destroy(ChessEngine* e) {
    delete e;
}

int chess_load_fen(ChessEngine* e, const char* fen) {
    if (!e || !fen) return 0;
    ensure_init(e);
    return e->board.load_fen(fen) ? 1 : 0;
}

int chess_side_to_move(ChessEngine* e) {
    if (!e) return 0;
    return e->board.get_side_to_move() == engine::Color::BLACK ? 1 : 0;
}

int chess_search(ChessEngine* e, int movetime_ms, char* out_uci, int out_len) {
    if (!e || !out_uci || out_len < 8) return std::numeric_limits<int>::min();
    ensure_init(e);
    engine::SearchLimits lim;
    lim.movetime_ms = movetime_ms > 0 ? movetime_ms : 500;
    lim.use_clock = true;
    auto r = e->search.search(e->board, lim);
    if (r.best.data == 0) {
        out_uci[0] = '\0';
        return std::numeric_limits<int>::min();
    }
    std::string u = engine::move_to_uci(r.best);
    std::snprintf(out_uci, static_cast<size_t>(out_len), "%s", u.c_str());
    return r.score;
}

int chess_make_uci(ChessEngine* e, const char* uci) {
    if (!e || !uci) return 0;
    ensure_init(e);
    engine::Move m = engine::move_from_uci(e->board, uci);
    if (m.data == 0) return 0;
    e->board.make_move(m);
    return 1;
}

int chess_legal_moves(ChessEngine* e, char* out_uci, int out_len) {
    if (!e || !out_uci || out_len < 1) return -1;
    ensure_init(e);
    auto moves = engine::MoveGen::generate_legal_moves(e->board);
    std::string all;
    for (size_t i = 0; i < moves.size(); i++) {
        if (i) all.push_back(' ');
        all += engine::move_to_uci(moves[i]);
    }
    if (static_cast<int>(all.size()) + 1 > out_len) return -1;
    std::snprintf(out_uci, static_cast<size_t>(out_len), "%s", all.c_str());
    return static_cast<int>(moves.size());
}

int chess_result(ChessEngine* e) {
    if (!e) return 3;
    using engine::GameResult;
    auto r = engine::position_result(e->board);
    if (r == GameResult::Ongoing) return 0;
    if (r == GameResult::WhiteMates) return 1;
    if (r == GameResult::BlackMates) return 2;
    return 3;
}
