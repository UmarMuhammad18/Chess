#include "app.hpp"
#include <SDL2/SDL.h>

namespace platform {
    bool App::init() {
        return sdl.init("C++ Chess", 1120, 800);
    }
    void App::poll_events() {
        input_sys.reset_frame();
        SDL_Event e;
        while (SDL_PollEvent(&e) != 0) {
            input_sys.update(e);
        }
    }
    void App::window_size(int& w, int& h) const {
        SDL_GetWindowSize(sdl.get_window(), &w, &h);
    }
}
