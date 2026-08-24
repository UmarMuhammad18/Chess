#pragma once
#include "sdl_context.hpp"
#include "input.hpp"

namespace platform {
    class App {
    public:
        bool init();
        void poll_events();
        bool is_running() const { return !input_sys.quit_requested(); }
        SDLContext& get_sdl() { return sdl; }
        const Input& get_input() const { return input_sys; }
        void window_size(int& w, int& h) const;
    private:
        SDLContext sdl;
        Input input_sys;
    };
}
