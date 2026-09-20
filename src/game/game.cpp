#include "game.hpp"
#include "../engine/movegen.hpp"
#include "../engine/utils.hpp"
#include "../engine/zobrist.hpp"
#include <algorithm>
#include <cstdio>
#include <sstream>

namespace game {
    namespace {
        const char* START_FEN =
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    }

    Game::~Game() { stop_ai(); }

    bool Game::init() {
        if (!app.init()) return false;
        if (!renderer.init()) return false;
        engine::MoveGen::init();
        engine::Zobrist::init();
        new_game();
        return true;
    }

    void Game::new_game() {
        stop_ai();
        board.load_fen(START_FEN);
        selected_sq = engine::Square::NONE;
        last_move = engine::Move{};
        move_stack.clear();
        san_list.clear();
        result = engine::GameResult::Ongoing;
        promo_pending = false;
        search.tt.clear();
    }

    void Game::stop_ai() {
        search.request_stop();
        if (ai_thread.joinable()) ai_thread.join();
        ai_busy = false;
        ai_done = false;
    }

    void Game::play_move(engine::Move m) {
        if (m.data == 0) return;
        san_list.push_back(engine::move_to_san(board, m));
        board.make_move(m);
        move_stack.push_back(m);
        last_move = m;
        selected_sq = engine::Square::NONE;
        promo_pending = false;
        result = engine::position_result(board);
    }

    void Game::undo() {
        stop_ai();
        if (move_stack.empty()) return;
        result = engine::GameResult::Ongoing;
        promo_pending = false;
        bool took_human = false;
        while (!move_stack.empty()) {
            engine::Color who = engine::opposite(board.get_side_to_move());
            board.unmake_move(move_stack.back());
            move_stack.pop_back();
            if (!san_list.empty()) san_list.pop_back();
            if (who == human_color) took_human = true;
            if (took_human && board.get_side_to_move() == human_color) break;
            if (move_stack.empty()) break;
        }
        last_move = move_stack.empty() ? engine::Move{} : move_stack.back();
        selected_sq = engine::Square::NONE;
        result = engine::position_result(board);
    }

    void Game::resign() {
        if (result != engine::GameResult::Ongoing) return;
        stop_ai();
        result = (human_color == engine::Color::WHITE)
                     ? engine::GameResult::WhiteResigns
                     : engine::GameResult::BlackResigns;
    }

    void Game::start_ai() {
        if (ai_busy || result != engine::GameResult::Ongoing) return;
        if (board.get_side_to_move() == human_color) return;
        stop_ai();
        search.clear_stop();
        ai_busy = true;
        ai_done = false;
        engine::Board snap = board;
        int ms = think_ms;
        ai_thread = std::thread([this, snap, ms]() mutable {
            engine::SearchLimits lim;
            lim.movetime_ms = ms;
            engine::SearchResult r = search.search(snap, lim);
            { std::lock_guard<std::mutex> lock(ai_mutex); ai_result = r; }
            ai_done = true;
            ai_busy = false;
        });
    }

    void Game::apply_ai_if_ready() {
        if (!ai_done) return;
        if (ai_thread.joinable()) ai_thread.join();
        ai_done = false;
        engine::SearchResult r;
        { std::lock_guard<std::mutex> lock(ai_mutex); r = ai_result; }
        if (result == engine::GameResult::Ongoing && r.best.data != 0)
            play_move(r.best);
    }

    void Game::export_pgn() {
        std::string white = (human_color == engine::Color::WHITE) ? "Human" : "Engine";
        std::string black = (human_color == engine::Color::BLACK) ? "Human" : "Engine";
        std::string pgn = engine::moves_to_pgn(san_list, result, white, black);
        FILE* f = std::fopen("game.pgn", "w");
        if (f) { std::fwrite(pgn.data(), 1, pgn.size(), f); std::fclose(f); }
        std::printf("\n===== PGN =====\n%s===== END =====\n", pgn.c_str());
    }

    void Game::start_analysis() {
        if (ai_busy) return;
        stop_ai();
        search.clear_stop();
        ai_busy = true;
        ai_done = false;
        engine::Board snap = board;
        ai_thread = std::thread([this, snap]() mutable {
            engine::SearchLimits lim;
            lim.use_clock = false;
            lim.max_depth = 12;
            engine::SearchResult r = search.search(snap, lim);
            { std::lock_guard<std::mutex> lock(ai_mutex); ai_result = r; }
            ai_done = true;
            ai_busy = false;
        });
    }

    void Game::handle_board_click(int mx, int my) {
        if (result != engine::GameResult::Ongoing) return;
        if (board.get_side_to_move() != human_color) return;
        if (ai_busy) return;
        engine::Square clicked = renderer.pixel_to_square(mx, my);
        if (clicked == engine::Square::NONE) return;

        if (selected_sq == engine::Square::NONE) {
            if (board.color_on(clicked) == human_color) selected_sq = clicked;
        } else if (clicked == selected_sq) {
            selected_sq = engine::Square::NONE;
        } else {
            std::vector<engine::Move> matches;
            for (const auto& m : engine::MoveGen::generate_legal_moves(board)) {
                if (m.get_from() == selected_sq && m.get_to() == clicked)
                    matches.push_back(m);
            }
            if (matches.empty()) {
                if (board.color_on(clicked) == human_color) selected_sq = clicked;
                else selected_sq = engine::Square::NONE;
            } else if (matches.size() == 1) {
                play_move(matches[0]);
            } else {
                promo_pending = true;
                promo_from = selected_sq;
                promo_to = clicked;
                selected_sq = engine::Square::NONE;
            }
        }
    }

    bool Game::button(int x, int y, int w, int h, const char* label, bool active,
                      int mx, int my, bool clicked) {
        bool hover = mx >= x && mx < x + w && my >= y && my < y + h;
        float r = 0.20f, g = 0.32f, b = 0.44f;
        if (active) { r = 0.22f; g = 0.52f; b = 0.38f; }
        if (hover) { r += 0.08f; g += 0.08f; b += 0.08f; }
        renderer.draw_rect(x, y, w, h, r, g, b, 1);
        renderer.draw_rect(x, y, w, 2, 0.05f, 0.06f, 0.08f, 1);
        int tw = renderer.text_width(label, 2);
        renderer.draw_text(x + std::max(4, (w - tw) / 2), y + (h - 14) / 2, label, 2, 0.95f, 0.96f, 0.94f);
        return clicked && hover;
    }

    static bool hit(int x, int y, int w, int h, int mx, int my) {
        return mx >= x && mx < x + w && my >= y && my < y + h;
    }

    void Game::handle_input() {
        const auto& in = app.get_input();
        int mx, my;
        in.get_mouse_pos(mx, my);
        bool clicked = in.is_mouse_just_pressed();

        if (in.key_new()) new_game();
        if (in.key_undo()) undo();
        if (in.key_resign()) resign();
        if (!clicked) return;

        int px = panel_x + 16;
        int pw = win_w - panel_x - 32;
        int y = 86;
        if (hit(px, y, pw, 36, mx, my)) { new_game(); return; }
        y += 44;
        if (hit(px, y, pw, 36, mx, my)) { undo(); return; }
        y += 44;
        if (hit(px, y, pw, 36, mx, my)) { resign(); return; }
        y += 56;
        int hw = (pw - 8) / 2;
        if (hit(px, y, hw, 32, mx, my)) { human_color = engine::Color::WHITE; new_game(); return; }
        if (hit(px + hw + 8, y, hw, 32, mx, my)) { human_color = engine::Color::BLACK; new_game(); return; }
        y += 48;
        int tw = (pw - 18) / 4;
        const int times[] = {500, 1000, 2000, 5000};
        for (int i = 0; i < 4; i++) {
            if (hit(px + i * (tw + 6), y, tw, 28, mx, my)) { think_ms = times[i]; return; }
        }
        y += 40;
        if (hit(px, y, pw, 32, mx, my)) {
            analysis_mode = !analysis_mode;
            if (analysis_mode) { stop_ai(); start_analysis(); }
            else stop_ai();
            return;
        }
        y += 40;
        if (hit(px, y, pw, 32, mx, my)) { export_pgn(); return; }

        if (promo_pending) {
            const engine::Piece pieces[] = {
                engine::Piece::QUEEN, engine::Piece::ROOK,
                engine::Piece::BISHOP, engine::Piece::KNIGHT
            };
            int pw4 = (pw - 18) / 4;
            for (int i = 0; i < 4; i++) {
                if (hit(px + i * (pw4 + 6), 320, pw4, 36, mx, my)) {
                    for (const auto& m : engine::MoveGen::generate_legal_moves(board)) {
                        if (m.get_from() == promo_from && m.get_to() == promo_to &&
                            m.promo_piece() == pieces[i]) {
                            play_move(m);
                            break;
                        }
                    }
                    promo_pending = false;
                    return;
                }
            }
        }
        handle_board_click(mx, my);
    }

    void Game::draw() {
        renderer.set_flipped(human_color == engine::Color::BLACK);
        renderer.set_board_size(board_px);
        renderer.begin_frame(win_w, win_h);

        renderer.draw_rect(0, 0, win_w, win_h, 0.10f, 0.11f, 0.13f, 1);
        renderer.draw_board();

        if (last_move.data != 0) {
            int f = static_cast<int>(last_move.get_from());
            int t = static_cast<int>(last_move.get_to());
            renderer.draw_square_tint(f % 8, f / 8, 0.90f, 0.80f, 0.20f, 0.35f);
            renderer.draw_square_tint(t % 8, t / 8, 0.90f, 0.80f, 0.20f, 0.40f);
        }
        if (selected_sq != engine::Square::NONE) {
            int s = static_cast<int>(selected_sq);
            renderer.draw_square_tint(s % 8, s / 8, 0.30f, 0.55f, 0.90f, 0.40f);
            auto legal = engine::MoveGen::generate_legal_moves(board);
            for (const auto& m : legal) {
                if (m.get_from() != selected_sq) continue;
                int ts = static_cast<int>(m.get_to());
                if (m.is_capture())
                    renderer.draw_square_tint(ts % 8, ts / 8, 0.85f, 0.25f, 0.20f, 0.40f);
                else
                    renderer.draw_dot(ts % 8, ts / 8, 0.25f, 0.55f, 0.30f);
            }
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

        int mx, my;
        app.get_input().get_mouse_pos(mx, my);
        bool clicked = false;

        renderer.draw_rect(panel_x, 0, win_w - panel_x, win_h, 0.13f, 0.14f, 0.17f, 1);
        renderer.draw_rect(panel_x, 0, 2, win_h, 0.08f, 0.08f, 0.10f, 1);

        int px = panel_x + 16;
        renderer.draw_text(px, 18, "C++ CHESS", 3, 0.93f, 0.84f, 0.45f);

        std::string status;
        if (ai_busy) status = analysis_mode ? "Analysing..." : "AI thinking...";
        else if (result != engine::GameResult::Ongoing) status = engine::result_string(result);
        else if (engine::MoveGen::in_check(board))
            status = board.get_side_to_move() == engine::Color::WHITE ? "White in check" : "Black in check";
        else
            status = board.get_side_to_move() == engine::Color::WHITE ? "White to move" : "Black to move";
        float sr = 0.90f, sg = 0.90f, sb = 0.88f;
        if (result != engine::GameResult::Ongoing) { sr = 0.95f; sg = 0.75f; sb = 0.35f; }
        if (ai_busy) { sr = 0.55f; sg = 0.78f; sb = 0.95f; }
        renderer.draw_text(px, 48, status.c_str(), 2, sr, sg, sb);

        int y = 86;
        int pw = win_w - panel_x - 32;
        button(px, y, pw, 36, "New Game", false, mx, my, clicked); y += 44;
        button(px, y, pw, 36, "Undo  (U)", false, mx, my, clicked); y += 44;
        button(px, y, pw, 36, "Resign  (R)", false, mx, my, clicked); y += 56;
        renderer.draw_text(px, y - 18, "Play as", 2, 0.65f, 0.66f, 0.68f);
        int hw = (pw - 8) / 2;
        button(px, y, hw, 32, "White", human_color == engine::Color::WHITE, mx, my, clicked);
        button(px + hw + 8, y, hw, 32, "Black", human_color == engine::Color::BLACK, mx, my, clicked);
        y += 48;
        renderer.draw_text(px, y - 16, "Think time", 2, 0.65f, 0.66f, 0.68f);
        int tw = (pw - 18) / 4;
        const int times[] = {500, 1000, 2000, 5000};
        const char* labels[] = {"0.5s", "1s", "2s", "5s"};
        for (int i = 0; i < 4; i++)
            button(px + i * (tw + 6), y, tw, 28, labels[i], think_ms == times[i], mx, my, clicked);

        y += 40;
        button(px, y, pw, 32, analysis_mode ? "Analysis: ON" : "Analysis: OFF",
               analysis_mode, mx, my, clicked);
        y += 40;
        button(px, y, pw, 32, "Export PGN", false, mx, my, clicked);

        if (promo_pending) {
            y += 40;
            renderer.draw_text(px, y, "Promote to:", 2, 0.90f, 0.80f, 0.40f);
            y += 20;
            int pw4 = (pw - 18) / 4;
            const char* plabels[] = {"Q", "R", "B", "N"};
            for (int i = 0; i < 4; i++)
                button(px + i * (pw4 + 6), y, pw4, 36, plabels[i], false, mx, my, clicked);
            y += 44;
        }

        y += 44;
        {
            std::lock_guard<std::mutex> lock(ai_mutex);
            if (ai_result.depth > 0) {
                float t = 0.5f + static_cast<float>(ai_result.score) / 800.0f;
                if (t < 0.f) t = 0.f;
                if (t > 1.f) t = 1.f;
                int bar_w = pw - 8;
                int fill = static_cast<int>(t * bar_w);
                renderer.draw_rect(px, y, bar_w, 10, 0.25f, 0.25f, 0.28f, 1);
                renderer.draw_rect(px, y, fill, 10, 0.85f, 0.85f, 0.80f, 1);
                y += 16;

                char buf[96];
                std::snprintf(buf, sizeof(buf), "d%d  %+d cp  %llu n",
                              ai_result.depth, ai_result.score,
                              static_cast<unsigned long long>(ai_result.nodes));
                renderer.draw_text(px, y, buf, 2, 0.70f, 0.78f, 0.70f);
                y += 20;
                if (!ai_result.pv.empty()) {
                    std::string pv = "pv";
                    for (size_t i = 0; i < ai_result.pv.size() && i < 8; i++)
                        pv += " " + engine::move_to_uci(ai_result.pv[i]);
                    renderer.draw_text(px, y, pv.c_str(), 2, 0.55f, 0.60f, 0.58f);
                    y += 20;
                }
            }
        }

        y += 12;
        renderer.draw_text(px, y, "Moves", 2, 0.65f, 0.66f, 0.68f);
        y += 20;
        int start = 0;
        int max_lines = std::max(1, (win_h - y - 16) / 16);
        int total_plies = static_cast<int>(san_list.size());
        int total_moves = (total_plies + 1) / 2;
        if (total_moves > max_lines) start = (total_moves - max_lines) * 2;
        int move_no = start / 2 + 1;
        for (int i = start; i < total_plies; i += 2) {
            std::ostringstream oss;
            oss << move_no++ << ". " << san_list[i];
            if (i + 1 < total_plies) oss << "  " << san_list[i + 1];
            renderer.draw_text(px, y, oss.str().c_str(), 2, 0.88f, 0.88f, 0.86f);
            y += 16;
            if (y > win_h - 18) break;
        }

        renderer.end_frame(app.get_sdl());
    }

    void Game::loop() {
        while (app.is_running()) {
            app.poll_events();
            app.window_size(win_w, win_h);
            board_px = std::min(win_h, std::max(400, win_w - 320));
            panel_x = board_px;

            handle_input();
            if (!analysis_mode) {
                apply_ai_if_ready();
                if (result == engine::GameResult::Ongoing &&
                    board.get_side_to_move() != human_color && !ai_busy && !ai_done)
                    start_ai();
            } else if (ai_done && ai_thread.joinable()) {
                ai_thread.join();
                ai_done = false;
            }
            draw();
        }
        stop_ai();
    }
}
