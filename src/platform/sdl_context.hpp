#pragma once
#include <SDL2/SDL.h>
#include <string>

namespace platform {
    class SDLContext {
    public:
        SDLContext();
        ~SDLContext();
        bool init(const std::string& title, int width, int height);
        void swap_buffers();
        SDL_Window* get_window() const { return window; }
    private:
        SDL_Window* window{nullptr};
        SDL_GLContext gl_context{nullptr};
    };
}
