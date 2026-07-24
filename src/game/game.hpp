#pragma once
#include "../engine/board.hpp"
#include "../engine/search.hpp"
#include "../platform/app.hpp"
#include "../render/renderer.hpp"

namespace game {
    class Game {
    public:
        bool init();
        void loop();
    private:
        engine::Board board;
        engine::Search search;
        platform::App app;
        render::Renderer renderer;
        
        engine::Square selected_sq{engine::Square::NONE};
    };
}
