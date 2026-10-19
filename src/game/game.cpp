#include "game.hpp"
#include "../engine/movegen.hpp"
#include "../engine/notation.hpp"
#include "../engine/utils.hpp"
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>

namespace game {
    namespace {
        constexpr float PANEL_WIDTH   = 360.0f;
        constexpr float MARGIN        = 20.0f;
        constexpr float EVAL_BAR_W    = 16.0f;
        constexpr float EVAL_BAR_GAP  = 10.0f;

        constexpr render::Color LAST_MOVE_COLOR  {0.95f, 0.85f, 0.25f, 0.42f};
        constexpr render::Color SELECTED_COLOR   {0.40f, 0.75f, 0.30f, 0.55f};
        constexpr render::Color HINT_COLOR       {0.08f, 0.08f, 0.08f, 0.28f};
        constexpr render::Color CHECK_COLOR      {0.95f, 0.15f, 0.10f, 0.70f};
        constexpr render::Color HOVER_COLOR      {1.00f, 1.00f, 1.00f, 0.18f};

        // Rasterise the UI font at the display's pixel density so text stays sharp on HiDPI screens
        void load_ui_font(float pixel_ratio) {
            ImGuiIO& io = ImGui::GetIO();
            const float size = 15.0f;
            const char* candidates[] = {
                "C:/Windows/Fonts/segoeui.ttf",
                "/System/Library/Fonts/Supplemental/Arial.ttf",
                "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                "/usr/share/fonts/TTF/DejaVuSans.ttf",
            };
            ImFont* font = nullptr;
            for (const char* path : candidates) {
                if (std::ifstream(path).good()) {
                    font = io.Fonts->AddFontFromFileTTF(path, size * pixel_ratio);
                    if (font) break;
                }
            }
            if (!font) {
                ImFontConfig config;
                config.SizePixels = 13.0f * pixel_ratio;
                io.Fonts->AddFontDefault(&config);
            }
            io.FontGlobalScale = 1.0f / pixel_ratio;
        }

        std::string asset_directory() {
            // Assets are copied next to the executable; fall back to the working directory
            if (char* base = SDL_GetBasePath()) {
                std::string dir = std::string(base) + "assets/";
                SDL_free(base);
                if (std::ifstream(dir + "shaders/basic.vert").good()) return dir;
            }
            return "assets/";
        }

        // Score as text from White's point of view, e.g. "+0.35" or "#-3"
        std::string format_eval(int white_score, bool mate, int white_mate_in) {
            char buf[32];
            if (mate) std::snprintf(buf, sizeof(buf), "#%d", white_mate_in);
            else      std::snprintf(buf, sizeof(buf), "%+.2f", white_score / 100.0);
            return buf;
        }
    }

    Game::Game() : search(64) {}

    Game::~Game() {
        stop_engine();
        if (imgui_ready) {
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplSDL2_Shutdown();
            ImGui::DestroyContext();
        }
    }

    bool Game::init() {
        if (!app.init()) return false;
        if (!renderer.init(asset_directory())) return false;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 6.0f;
        style.FrameRounding = 4.0f;
        style.GrabRounding = 4.0f;
        style.WindowBorderSize = 0.0f;
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.11f, 0.13f, 1.0f);

        platform::SDLContext& sdl = app.get_sdl();
        int ww, wh, dw, dh;
        sdl.get_window_size(ww, wh);
        sdl.get_drawable_size(dw, dh);
        load_ui_font(ww > 0 ? std::max(1.0f, static_cast<float>(dw) / static_cast<float>(ww)) : 1.0f);
        ImGui_ImplSDL2_InitForOpenGL(sdl.get_window(), sdl.get_gl_context());
        ImGui_ImplOpenGL3_Init("#version 330 core");
        app.set_event_hook([](const SDL_Event& e) { ImGui_ImplSDL2_ProcessEvent(&e); });
        imgui_ready = true;

        engine::MoveGen::init();
        engine::Board start;
        start.load_fen(engine::START_FEN);
        new_game(start);
        return true;
    }

    // ───────────────────────────── game state ─────────────────────────────

    void Game::new_game(const engine::Board& start) {
        stop_engine();
        board = start;
        start_board = start;
        move_history.clear();
        san_history.clear();
        selected = engine::Square::NONE;
        dragging = false;
        promo_pending = false;
        have_info = false;
        {
            std::lock_guard<std::mutex> lock(info_mutex);
            live_info = engine::SearchInfo{};
        }
        paused = false;
        search.new_game();
        refresh_position();
    }

    void Game::play_move(engine::Move m) {
        san_history.push_back(engine::move_to_san(board, m));
        board.make_move(m);
        move_history.push_back(m);
        selected = engine::Square::NONE;
        dragging = false;
        scroll_moves = true;
        refresh_position();
    }

    void Game::undo() {
        stop_engine();
        if (move_history.empty()) return;

        auto pop = [this] {
            board.unmake_move(move_history.back());
            move_history.pop_back();
            san_history.pop_back();
        };
        pop();
        // Against the engine, step back to the human's turn
        bool has_human = players[0] == PlayerType::Human || players[1] == PlayerType::Human;
        if (has_human) {
            while (!move_history.empty() && is_engine_turn()) pop();
        } else {
            paused = true;
        }
        selected = engine::Square::NONE;
        promo_pending = false;
        refresh_position();
    }

    void Game::refresh_position() {
        legal_moves = engine::MoveGen::generate_legal_moves(board);
        std::snprintf(fen_buffer, sizeof(fen_buffer), "%s", board.get_fen().c_str());

        if (legal_moves.empty()) {
            if (engine::MoveGen::in_check(board))
                result = board.get_side_to_move() == engine::Color::WHITE ? GameResult::BlackMates : GameResult::WhiteMates;
            else
                result = GameResult::Stalemate;
        } else if (board.is_fifty_move_draw()) {
            result = GameResult::FiftyMoves;
        } else if (board.repetition_count() >= 2) {
            result = GameResult::Repetition;
        } else if (board.is_insufficient_material()) {
            result = GameResult::InsufficientMaterial;
        } else {
            result = GameResult::Ongoing;
        }
    }

    bool Game::is_engine_turn() const {
        return players[static_cast<int>(board.get_side_to_move())] == PlayerType::Engine;
    }

    std::string Game::result_text() const {
        switch (result) {
            case GameResult::WhiteMates:           return "Checkmate - White wins";
            case GameResult::BlackMates:           return "Checkmate - Black wins";
            case GameResult::Stalemate:            return "Draw by stalemate";
            case GameResult::FiftyMoves:           return "Draw by the 50-move rule";
            case GameResult::Repetition:           return "Draw by threefold repetition";
            case GameResult::InsufficientMaterial: return "Draw by insufficient material";
            default:                               return "";
        }
    }

    std::string Game::build_pgn() const {
        const char* res = "*";
        if (result == GameResult::WhiteMates) res = "1-0";
        else if (result == GameResult::BlackMates) res = "0-1";
        else if (result != GameResult::Ongoing) res = "1/2-1/2";

        auto name = [this](int c) { return players[c] == PlayerType::Human ? "Human" : "ChessEngine"; };
        std::ostringstream pgn;
        pgn << "[Event \"Casual game\"]\n[White \"" << name(0) << "\"]\n[Black \"" << name(1) << "\"]\n"
            << "[Result \"" << res << "\"]\n";
        if (start_board.get_fen() != engine::START_FEN)
            pgn << "[SetUp \"1\"]\n[FEN \"" << start_board.get_fen() << "\"]\n";
        pgn << "\n";

        int number = start_board.get_full_move_number();
        bool white = start_board.get_side_to_move() == engine::Color::WHITE;
        for (size_t i = 0; i < san_history.size(); i++) {
            if (white)       pgn << number << ". ";
            else if (i == 0) pgn << number << "... ";
            pgn << san_history[i] << ' ';
            if (!white) number++;
            white = !white;
        }
        pgn << res << "\n";
        return pgn.str();
    }

    // ───────────────────────────── engine thread ─────────────────────────────

    void Game::start_engine() {
        search_root = board;
        engine::SearchLimits limits;
        limits.movetime = static_cast<int64_t>(think_seconds * 1000.0f);
        limits.depth = max_depth;

        engine_finished = false;
        engine_running = true;
        have_info = false;
        {
            std::lock_guard<std::mutex> lock(info_mutex);
            live_info = engine::SearchInfo{};
        }
        engine::Board root = board;
        worker = std::thread([this, root, limits]() mutable {
            engine::Move best = search.think(root, limits, [this](const engine::SearchInfo& info) {
                std::lock_guard<std::mutex> lock(info_mutex);
                live_info = info;
            });
            {
                std::lock_guard<std::mutex> lock(info_mutex);
                engine_result = best;
            }
            engine_finished = true;
        });
    }

    void Game::stop_engine() {
        if (!worker.joinable()) return;
        // think() clears the stop flag when it starts, so keep asking until it has finished
        while (!engine_finished.load()) {
            search.request_stop();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        worker.join();
        engine_running = false;
        engine_finished = false;
    }

    void Game::poll_engine() {
        if (!engine_running || !engine_finished) return;
        worker.join();
        engine_running = false;
        engine_finished = false;

        engine::Move m;
        {
            std::lock_guard<std::mutex> lock(info_mutex);
            m = engine_result;
        }
        if (!m.is_null()) play_move(m);
    }

    // ───────────────────────────── input ─────────────────────────────

    void Game::update_layout() {
        platform::SDLContext& sdl = app.get_sdl();
        sdl.get_window_size(win_w, win_h);
        sdl.get_drawable_size(fb_w, fb_h);
        dpi_scale = win_w > 0 ? static_cast<float>(fb_w) / static_cast<float>(win_w) : 1.0f;

        panel_w = PANEL_WIDTH;
        float left = MARGIN + EVAL_BAR_W + EVAL_BAR_GAP;
        float avail_w = static_cast<float>(win_w) - panel_w - left - 2 * MARGIN;
        float avail_h = static_cast<float>(win_h) - 2 * MARGIN;
        float board_size = std::max(160.0f, std::min(avail_w, avail_h));
        square = std::floor(board_size / 8.0f);
        board_size = square * 8.0f;

        eval_bar_x = MARGIN;
        board_x = left;
        board_y = std::floor((static_cast<float>(win_h) - board_size) * 0.5f);
        panel_x = board_x + board_size + MARGIN;
        panel_w = static_cast<float>(win_w) - panel_x;
    }

    bool Game::square_at(int wx, int wy, engine::Square& out) const {
        float fx = (static_cast<float>(wx) - board_x) / square;
        float fy = (static_cast<float>(wy) - board_y) / square;
        if (fx < 0 || fy < 0 || fx >= 8 || fy >= 8) return false;
        int col = static_cast<int>(fx), row = static_cast<int>(fy);
        int file = flipped ? 7 - col : col;
        int rank = flipped ? row : 7 - row;
        out = static_cast<engine::Square>(rank * 8 + file);
        return true;
    }

    render::Rect Game::square_rect_px(engine::Square sq) const {
        int s = static_cast<int>(sq);
        int file = s % 8, rank = s / 8;
        int col = flipped ? 7 - file : file;
        int row = flipped ? rank : 7 - rank;
        float x = (board_x + static_cast<float>(col) * square) * dpi_scale;
        float y = (board_y + static_cast<float>(row) * square) * dpi_scale;
        return {x, y, square * dpi_scale, square * dpi_scale};
    }

    void Game::try_move(engine::Square from, engine::Square to) {
        engine::Move match{};
        bool promotion = false;
        for (const engine::Move& m : legal_moves) {
            if (m.get_from() == from && m.get_to() == to) {
                match = m;
                promotion = promotion || m.is_promotion();
            }
        }
        if (match.is_null()) return;
        if (promotion) {
            promo_pending = true;
            promo_from = from;
            promo_to = to;
            return;
        }
        play_move(match);
    }

    void Game::handle_board_input() {
        const platform::Input& input = app.get_input();
        bool can_move = result == GameResult::Ongoing && !is_engine_turn() && !engine_running && !promo_pending;
        if (!can_move) {
            dragging = false;
            if (result != GameResult::Ongoing || promo_pending) selected = engine::Square::NONE;
            return;
        }
        if (ImGui::GetIO().WantCaptureMouse && !dragging) return;

        int mx, my;
        input.get_mouse_pos(mx, my);
        engine::Square sq;
        bool on_board = square_at(mx, my, sq);
        engine::Color stm = board.get_side_to_move();

        auto is_target = [this](engine::Square from, engine::Square to) {
            for (const engine::Move& m : legal_moves)
                if (m.get_from() == from && m.get_to() == to) return true;
            return false;
        };

        if (input.is_mouse_just_pressed()) {
            deselect_on_release = false;
            if (!on_board) {
                selected = engine::Square::NONE;
            } else if (selected != engine::Square::NONE && is_target(selected, sq)) {
                try_move(selected, sq);
                if (!promo_pending) selected = engine::Square::NONE;
            } else if (board.color_on(sq) == stm) {
                deselect_on_release = (sq == selected);
                selected = sq;
                dragging = true;
            } else {
                selected = engine::Square::NONE;
            }
        }

        if (input.is_mouse_just_released() && dragging) {
            dragging = false;
            if (on_board && sq != selected && is_target(selected, sq)) {
                try_move(selected, sq);
                if (!promo_pending) selected = engine::Square::NONE;
            } else if (on_board && sq == selected && deselect_on_release) {
                selected = engine::Square::NONE;
            }
        }
    }

    // ───────────────────────────── drawing ─────────────────────────────

    void Game::draw_scene() {
        float s = dpi_scale;
        float board_px = square * 8.0f * s;
        render::Rect board_rect{board_x * s, board_y * s, board_px, board_px};

        // Frame and board
        float border = 4.0f * s;
        renderer.fill_rect({board_rect.x - border, board_rect.y - border, board_px + 2 * border, board_px + 2 * border},
                           {0.05f, 0.05f, 0.06f, 1.0f});
        renderer.draw_board(board_rect);

        // Last move
        if (!move_history.empty()) {
            engine::Move last = move_history.back();
            renderer.fill_rect(square_rect_px(last.get_from()), LAST_MOVE_COLOR);
            renderer.fill_rect(square_rect_px(last.get_to()), LAST_MOVE_COLOR);
        }

        // King in check
        if (engine::MoveGen::in_check(board)) {
            engine::U64 king = board.get_pieces(engine::Piece::KING, board.get_side_to_move());
            if (king) {
                render::Rect r = square_rect_px(static_cast<engine::Square>(engine::lsb(king)));
                renderer.fill_disc(r, CHECK_COLOR);
            }
        }

        // Selection, hover target and legal destinations
        int mx, my;
        app.get_input().get_mouse_pos(mx, my);
        if (selected != engine::Square::NONE) {
            renderer.fill_rect(square_rect_px(selected), SELECTED_COLOR);
            engine::Square hover = engine::Square::NONE;
            bool hovering = dragging && square_at(mx, my, hover);
            for (const engine::Move& m : legal_moves) {
                if (m.get_from() != selected) continue;
                render::Rect r = square_rect_px(m.get_to());
                if (hovering && hover == m.get_to()) renderer.fill_rect(r, HOVER_COLOR);
                if (m.is_capture() && m.get_flag() != engine::MoveFlag::EnPassant) {
                    renderer.draw_ring(r, HINT_COLOR);
                } else {
                    float d = r.w * 0.32f;
                    renderer.fill_disc({r.x + (r.w - d) * 0.5f, r.y + (r.h - d) * 0.5f, d, d}, HINT_COLOR);
                }
            }
        }

        // Pieces (the dragged one is drawn last, under the cursor)
        for (int sq = 0; sq < 64; sq++) {
            engine::Square square_id = static_cast<engine::Square>(sq);
            engine::Piece p = board.piece_on(square_id);
            if (p == engine::Piece::NONE) continue;
            bool lifted = dragging && square_id == selected;
            renderer.draw_piece(static_cast<int>(p), static_cast<int>(board.color_on(square_id)),
                                square_rect_px(square_id), lifted ? 0.35f : 1.0f);
        }
        if (dragging && selected != engine::Square::NONE) {
            float size = square * s * 1.1f;
            render::Rect r{mx * s - size * 0.5f, my * s - size * 0.5f, size, size};
            renderer.draw_piece(static_cast<int>(board.piece_on(selected)), static_cast<int>(board.color_on(selected)), r);
        }

        // Evaluation bar (White's share grows from the bottom unless the board is flipped)
        float white_share = 0.5f;
        if (have_info) {
            std::lock_guard<std::mutex> lock(info_mutex);
            int white_score = search_root.get_side_to_move() == engine::Color::WHITE ? live_info.score : -live_info.score;
            if (live_info.is_mate()) white_share = white_score > 0 ? 1.0f : 0.0f;
            else white_share = 1.0f / (1.0f + std::exp(-static_cast<float>(white_score) / 300.0f));
        }
        float bar_x = eval_bar_x * s, bar_w = EVAL_BAR_W * s;
        float white_h = board_px * white_share;
        renderer.fill_rect({bar_x, board_rect.y, bar_w, board_px}, {0.20f, 0.20f, 0.22f, 1.0f});
        float white_y = flipped ? board_rect.y : board_rect.y + board_px - white_h;
        renderer.fill_rect({bar_x, white_y, bar_w, white_h}, {0.93f, 0.93f, 0.90f, 1.0f});
    }

    void Game::draw_coordinates() {
        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        ImU32 light = IM_COL32(240, 217, 181, 230), dark = IM_COL32(110, 75, 50, 230);
        for (int i = 0; i < 8; i++) {
            int file = flipped ? 7 - i : i;
            int rank = flipped ? i : 7 - i;
            // File letters along the bottom row, rank numbers down the left column
            char f[2] = {static_cast<char>('a' + file), 0};
            char r[2] = {static_cast<char>('1' + rank), 0};
            // a1 is a dark square: (file + rank) even means dark, so use the light label colour
            bool bottom_dark = (file + (flipped ? 7 : 0)) % 2 == 0;
            bool left_dark = ((flipped ? 7 : 0) + rank) % 2 == 0;
            dl->AddText(ImVec2(board_x + (i + 1) * square - 10.0f, board_y + 8 * square - 16.0f), bottom_dark ? light : dark, f);
            dl->AddText(ImVec2(board_x + 3.0f, board_y + i * square + 2.0f), left_dark ? light : dark, r);
        }
    }

    void Game::draw_overlays() {
        // Promotion picker
        if (promo_pending && !ImGui::IsPopupOpen("Promote pawn")) {
            ImGui::OpenPopup("Promote pawn");
        }
        ImGui::SetNextWindowPos(ImVec2(board_x + square * 4, board_y + square * 4), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("Promote pawn", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
            static const struct { const char* label; engine::Piece piece; } choices[] = {
                {"Queen", engine::Piece::QUEEN}, {"Rook", engine::Piece::ROOK},
                {"Bishop", engine::Piece::BISHOP}, {"Knight", engine::Piece::KNIGHT},
            };
            for (const auto& c : choices) {
                if (ImGui::Button(c.label, ImVec2(90, 36))) {
                    for (const engine::Move& m : legal_moves) {
                        if (m.get_from() == promo_from && m.get_to() == promo_to && m.promo_piece() == c.piece) {
                            promo_pending = false;
                            play_move(m);
                            break;
                        }
                    }
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
            }
            ImGui::NewLine();
            if (ImGui::Button("Cancel", ImVec2(-1, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                promo_pending = false;
                selected = engine::Square::NONE;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // Game over banner
        if (result != GameResult::Ongoing) {
            ImGui::SetNextWindowPos(ImVec2(board_x + square * 4, board_y + square * 4), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowBgAlpha(0.92f);
            ImGui::Begin("##result", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                              ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                                              ImGuiWindowFlags_NoFocusOnAppearing);
            ImGui::SetWindowFontScale(1.4f);
            ImGui::TextUnformatted(result_text().c_str());
            ImGui::SetWindowFontScale(1.0f);
            if (ImGui::Button("New game", ImVec2(-1, 0))) {
                engine::Board start;
                start.load_fen(engine::START_FEN);
                new_game(start);
            }
            if (ImGui::Button("Undo last move", ImVec2(-1, 0))) undo();
            ImGui::End();
        }
    }

    void Game::draw_panel() {
        ImGui::SetNextWindowPos(ImVec2(panel_x, 0));
        ImGui::SetNextWindowSize(ImVec2(panel_w, static_cast<float>(win_h)));
        ImGui::Begin("##panel", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);

        // Status
        ImGui::SetWindowFontScale(1.25f);
        ImGui::TextUnformatted("Chess Engine");
        ImGui::SetWindowFontScale(1.0f);
        if (result != GameResult::Ongoing) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "%s", result_text().c_str());
        } else {
            const char* side = board.get_side_to_move() == engine::Color::WHITE ? "White" : "Black";
            if (engine_running)  ImGui::Text("%s (engine) is thinking...", side);
            else if (paused && is_engine_turn()) ImGui::Text("%s to move - engine paused", side);
            else                 ImGui::Text("%s to move", side);
        }

        // Players
        ImGui::SeparatorText("Players");
        const char* kinds = "Human\0Engine\0";
        for (int c = 0; c < 2; c++) {
            int kind = static_cast<int>(players[c]);
            ImGui::SetNextItemWidth(140);
            if (ImGui::Combo(c == 0 ? "White" : "Black", &kind, kinds)) {
                // Changing who plays the side that is thinking cancels that search
                if (static_cast<int>(board.get_side_to_move()) == c) stop_engine();
                players[c] = static_cast<PlayerType>(kind);
            }
            if (c == 0) ImGui::SameLine();
        }
        ImGui::SetNextItemWidth(-90);
        ImGui::SliderFloat("Think time", &think_seconds, 0.1f, 15.0f, "%.1f s", ImGuiSliderFlags_Logarithmic);
        ImGui::SetNextItemWidth(-90);
        ImGui::SliderInt("Max depth", &max_depth, 1, 64);

        // Controls
        ImGui::SeparatorText("Game");
        float bw = (ImGui::GetContentRegionAvail().x - 2 * ImGui::GetStyle().ItemSpacing.x) / 3.0f;
        if (ImGui::Button("New game", ImVec2(bw, 0))) {
            engine::Board start;
            start.load_fen(engine::START_FEN);
            new_game(start);
        }
        ImGui::SameLine();
        ImGui::BeginDisabled(move_history.empty());
        if (ImGui::Button("Undo", ImVec2(bw, 0))) undo();
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Flip board", ImVec2(bw, 0))) flipped = !flipped;

        ImGui::BeginDisabled(!engine_running);
        if (ImGui::Button("Move now", ImVec2(bw, 0))) search.request_stop();
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button(paused ? "Resume" : "Pause", ImVec2(bw, 0))) {
            paused = !paused;
            if (paused) stop_engine();
        }
        ImGui::SameLine();
        if (ImGui::Button("Copy PGN", ImVec2(bw, 0))) {
            ImGui::SetClipboardText(build_pgn().c_str());
            status_message = "PGN copied to clipboard";
        }

        // Engine analysis
        ImGui::SeparatorText("Engine");
        if (have_info) {
            engine::SearchInfo info;
            {
                std::lock_guard<std::mutex> lock(info_mutex);
                info = live_info;
            }
            bool white_root = search_root.get_side_to_move() == engine::Color::WHITE;
            int white_score = white_root ? info.score : -info.score;
            int white_mate = white_root ? info.mate_in() : -info.mate_in();
            std::string eval = format_eval(white_score, info.is_mate(), white_mate);

            float frac = info.is_mate() ? (white_score > 0 ? 1.0f : 0.0f)
                                        : 1.0f / (1.0f + std::exp(-static_cast<float>(white_score) / 300.0f));
            ImGui::ProgressBar(frac, ImVec2(-1, 0), eval.c_str());

            int64_t nps = info.time_ms > 0 ? static_cast<int64_t>(info.nodes * 1000 / info.time_ms) : 0;
            ImGui::Text("Depth %d/%d   Nodes %llu", info.depth, info.seldepth, static_cast<unsigned long long>(info.nodes));
            ImGui::Text("Speed %.2f Mn/s   Time %.2fs   Hash %d%%",
                        static_cast<double>(nps) / 1e6, static_cast<double>(info.time_ms) / 1000.0, info.hashfull / 10);

            // Principal variation in SAN, replayed from the search's root position
            std::string pv;
            engine::Board walk = search_root;
            int number = walk.get_full_move_number();
            for (size_t i = 0; i < info.pv.size() && i < 16; i++) {
                bool white = walk.get_side_to_move() == engine::Color::WHITE;
                if (white)       pv += std::to_string(number) + ". ";
                else if (i == 0) pv += std::to_string(number) + "... ";
                pv += engine::move_to_san(walk, info.pv[i]) + " ";
                walk.make_move(info.pv[i]);
                if (!white) number++;
            }
            ImGui::TextWrapped("PV: %s", pv.c_str());
        } else {
            ImGui::TextDisabled("No analysis yet - the engine reports here while it thinks.");
        }

        // Move list
        ImGui::SeparatorText("Moves");
        float list_h = std::max(80.0f, ImGui::GetContentRegionAvail().y - 110.0f);
        if (ImGui::BeginChild("moves", ImVec2(0, list_h), ImGuiChildFlags_Borders)) {
            if (ImGui::BeginTable("movetable", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 34.0f);
                ImGui::TableSetupColumn("White");
                ImGui::TableSetupColumn("Black");
                int number = start_board.get_full_move_number();
                size_t i = 0;
                bool black_first = start_board.get_side_to_move() == engine::Color::BLACK;
                while (i < san_history.size()) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextDisabled("%d.", number);
                    ImGui::TableSetColumnIndex(1);
                    if (black_first && i == 0) {
                        ImGui::TextDisabled("...");
                    } else {
                        ImGui::TextUnformatted(san_history[i].c_str());
                        i++;
                    }
                    ImGui::TableSetColumnIndex(2);
                    if (i < san_history.size()) {
                        ImGui::TextUnformatted(san_history[i].c_str());
                        i++;
                    }
                    number++;
                }
                ImGui::EndTable();
            }
            if (scroll_moves) {
                ImGui::SetScrollHereY(1.0f);
                scroll_moves = false;
            }
        }
        ImGui::EndChild();

        // FEN
        ImGui::SeparatorText("Position (FEN)");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##fen", fen_buffer, sizeof(fen_buffer));
        if (ImGui::Button("Load FEN", ImVec2(bw, 0))) {
            engine::Board loaded;
            if (loaded.load_fen(fen_buffer)) {
                new_game(loaded);
                status_message = "Position loaded";
            } else {
                status_message = "Invalid FEN";
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Copy FEN", ImVec2(bw, 0))) {
            ImGui::SetClipboardText(board.get_fen().c_str());
            status_message = "FEN copied to clipboard";
        }
        if (!status_message.empty()) {
            ImGui::SameLine();
            ImGui::TextDisabled("%s", status_message.c_str());
        }
        ImGui::TextDisabled("Keys: F flip, Ctrl+Z undo, Ctrl+N new, Space move now");
        ImGui::End();
    }

    void Game::loop() {
        while (app.is_running()) {
            app.poll_events();
            poll_engine();

            if (result == GameResult::Ongoing && is_engine_turn() && !engine_running && !paused && !promo_pending)
                start_engine();
            if (!have_info) {
                std::lock_guard<std::mutex> lock(info_mutex);
                have_info = live_info.depth > 0;
            }

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();

            update_layout();
            handle_board_input();

            // Keyboard shortcuts (ignored while typing into a text field)
            ImGuiIO& io = ImGui::GetIO();
            if (!io.WantTextInput) {
                if (ImGui::IsKeyPressed(ImGuiKey_F, false)) flipped = !flipped;
                if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false)) undo();
                if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_N, false)) {
                    engine::Board start;
                    start.load_fen(engine::START_FEN);
                    new_game(start);
                }
                if (ImGui::IsKeyPressed(ImGuiKey_Space, false) && engine_running) search.request_stop();
            }

            draw_panel();
            draw_overlays();
            draw_coordinates();

            renderer.begin_frame(fb_w, fb_h);
            draw_scene();
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            renderer.end_frame(app.get_sdl());
        }
    }
}
