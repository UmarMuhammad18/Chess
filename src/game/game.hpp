#pragma once
#include "../engine/board.hpp"
#include "../engine/search.hpp"
#include "../platform/app.hpp"
#include "../render/renderer.hpp"
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace game {
    enum class PlayerType { Human, Engine };

    enum class GameResult { Ongoing, WhiteMates, BlackMates, Stalemate, FiftyMoves, Repetition, InsufficientMaterial };

    class Game {
    public:
        Game();
        ~Game();
        bool init();
        void loop();
    private:
        // ── Game state ──
        void new_game(const engine::Board& start);
        void play_move(engine::Move m);
        void undo();
        void refresh_position();
        bool is_engine_turn() const;
        std::string result_text() const;
        std::string build_pgn() const;

        // ── Engine worker thread ──
        void start_engine();
        void stop_engine();     // aborts and discards the result
        void poll_engine();     // applies a finished search's move

        // ── Input / UI ──
        void update_layout();
        void handle_board_input();
        void try_move(engine::Square from, engine::Square to);
        bool square_at(int wx, int wy, engine::Square& out) const;
        render::Rect square_rect_px(engine::Square sq) const;
        void draw_scene();
        void draw_panel();
        void draw_overlays();
        void draw_coordinates();

        // Platform/renderer first so they are destroyed last (GL objects need a live context)
        platform::App app;
        render::Renderer renderer;
        bool imgui_ready{false};

        engine::Board board;
        engine::Board start_board;
        std::vector<engine::Move> move_history;
        std::vector<std::string> san_history;
        engine::MoveList legal_moves;
        GameResult result{GameResult::Ongoing};
        PlayerType players[2]{PlayerType::Human, PlayerType::Engine};
        bool paused{false};
        bool flipped{false};

        // Selection, dragging and promotion
        engine::Square selected{engine::Square::NONE};
        bool dragging{false};
        bool deselect_on_release{false};
        bool promo_pending{false};
        engine::Square promo_from{engine::Square::NONE}, promo_to{engine::Square::NONE};

        // Engine thread state
        engine::Search search;
        std::thread worker;
        std::atomic<bool> engine_running{false};
        std::atomic<bool> engine_finished{false};
        std::mutex info_mutex;
        engine::SearchInfo live_info;      // guarded by info_mutex
        engine::Move engine_result;        // guarded by info_mutex
        engine::Board search_root;         // position the current/last search started from
        bool have_info{false};
        float think_seconds{1.0f};
        int max_depth{64};

        // Layout (window units) and the window→pixel scale
        float dpi_scale{1.0f};
        float board_x{0}, board_y{0}, square{80};
        float eval_bar_x{0};
        float panel_x{0}, panel_w{360};
        int fb_w{1}, fb_h{1}, win_w{1}, win_h{1};

        char fen_buffer[128]{};
        std::string status_message;
        bool scroll_moves{false};
    };
}
