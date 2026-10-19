#include "app.hpp"
#include <SDL2/SDL.h>

namespace platform {
    bool App::init() {
        return sdl.init("C++ Chess Engine", 1180, 820);
    }
    void App::poll_events() {
        input_sys.reset_frame();
        SDL_Event e;
        while (SDL_PollEvent(&e) != 0) {
            if (event_hook) event_hook(e);
            input_sys.update(e);
        }
    }
}
