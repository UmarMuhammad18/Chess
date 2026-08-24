#pragma once
#include "../engine/board.hpp"
#include "../engine/notation.hpp"
#include "../engine/search.hpp"
#include "../platform/app.hpp"
#include "../render/renderer.hpp"
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace game {
    class Game {
    public:
        Game() = default;
        ~Game();
        bool init();
        void loop();
    private:
        void new_game();
        void undo();
        void resign();
        void play_move(engine::Move m);
        void start_ai();
        void stop_ai();
        void apply_ai_if_ready();
        void handle_board_click(int mx, int my);
        void handle_input();
        void draw();
        bool button(int x, int y, int w, int h, const char* label, bool active,
                    int mx, int my, bool clicked);

        engine::Board board;
        engine::Search search;
        platform::App app;
        render::Renderer renderer;

        engine::Square selected_sq{engine::Square::NONE};
        engine::Color human_color{engine::Color::WHITE};
        engine::GameResult result{engine::GameResult::Ongoing};
        engine::Move last_move{};
        std::vector<engine::Move> move_stack;
        std::vector<std::string> san_list;
        int think_ms{1000};

        std::thread ai_thread;
        std::atomic<bool> ai_busy{false};
        std::atomic<bool> ai_done{false};
        engine::SearchResult ai_result;
        std::mutex ai_mutex;

        int win_w{1120};
        int win_h{800};
        int board_px{800};
        int panel_x{800};
    };
}
