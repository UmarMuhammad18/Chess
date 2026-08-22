#include "../engine/zobrist.hpp"
#include "game.hpp"
#include "../engine/movegen.hpp"
#include "../engine/utils.hpp"
#include <iostream>

namespace game {
    namespace {
        engine::Move find_legal_move(engine::Board& board, engine::Square from, engine::Square to) {
            engine::Move best{};
            bool found = false;
            for (const auto& m : engine::MoveGen::generate_legal_moves(board)) {
                if (m.get_from() != from || m.get_to() != to) continue;
                if (!found || m.promo_piece() == engine::Piece::QUEEN) {
                    best = m;
                    found = true;
                }
            }
            return best;
        }
    }

    bool Game::init() {
        if (!app.init()) return false;
        if (!renderer.init()) return false;
        
        engine::MoveGen::init();
        engine::Zobrist::init();
        board.load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        return true;
    }
    
    void Game::loop() {
        while (app.is_running()) {
            app.poll_events();
            
            // AI Turn
            if (board.get_side_to_move() == engine::Color::BLACK) {
                engine::Move best = search.search_best_move(board, 4);
                if (best.data != 0) {
                    board.make_move(best);
                }
            } 
            // Human Turn
            else {
                if (app.get_input().is_mouse_just_pressed()) {
                    int mx, my;
                    app.get_input().get_mouse_pos(mx, my);
                    
                    // SDL (0,0) is top-left; OpenGL draws rank 1 at the bottom
                    int file = mx / 100;
                    int rank = 7 - (my / 100);
                    if (file >= 0 && file < 8 && rank >= 0 && rank < 8) {
                        engine::Square clicked = static_cast<engine::Square>(rank * 8 + file);
                        
                        if (selected_sq == engine::Square::NONE) {
                            if (board.color_on(clicked) == board.get_side_to_move()) {
                                selected_sq = clicked;
                            }
                        } else if (clicked == selected_sq) {
                            selected_sq = engine::Square::NONE;
                        } else {
                            engine::Move legal = find_legal_move(board, selected_sq, clicked);
                            if (legal.data != 0) {
                                board.make_move(legal);
                                selected_sq = engine::Square::NONE;
                            } else if (board.color_on(clicked) == board.get_side_to_move()) {
                                selected_sq = clicked;
                            } else {
                                selected_sq = engine::Square::NONE;
                            }
                        }
                    }
                }
            }
            
            renderer.begin_frame();
            renderer.draw_board();
            
            if (selected_sq != engine::Square::NONE) {
                int sq = static_cast<int>(selected_sq);
                renderer.draw_highlight(sq % 8, sq / 8);
            }
            
            for (int c = 0; c < 2; c++) {
                engine::Color col = static_cast<engine::Color>(c);
                engine::U64 col_bb = board.get_pieces(col);
                for (int p = 0; p < 6; p++) {
                    engine::Piece piece = static_cast<engine::Piece>(p);
                    engine::U64 bb = board.get_pieces(piece) & col_bb;
                    while (bb) {
                        int sq = engine::lsb(bb);
                        engine::clear_bit(bb, static_cast<engine::Square>(sq));
                        renderer.draw_piece(p, c, sq % 8, sq / 8);
                    }
                }
            }
            renderer.end_frame(app.get_sdl());
        }
    }
}
