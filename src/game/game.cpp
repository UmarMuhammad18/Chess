#include "../engine/zobrist.hpp"
#include "game.hpp"
#include "../engine/movegen.hpp"
#include <iostream>

namespace game {
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
                    
                    // Convert screen to board (Assume 800x800, 100px squares)
                    int file = mx / 100;
                    int rank = my / 100; // Need to invert Y if rendering (0,0) is top-left, but OpenGL viewport handles Y from bottom.
                    // For scaffolding let's assume direct map:
                    engine::Square clicked = static_cast<engine::Square>(rank * 8 + file);
                    
                    if (selected_sq == engine::Square::NONE) {
                        selected_sq = clicked;
                    } else {
                        // Make move (Simplified: bypass legal check for scaffold)
                        board.make_move(engine::Move(selected_sq, clicked));
                        selected_sq = engine::Square::NONE;
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
