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
        SDL_GLContext get_gl_context() const { return gl_context; }

        // Size in screen coordinates (what mouse events use)
        void get_window_size(int& w, int& h) const { SDL_GetWindowSize(window, &w, &h); }
        // Size in pixels (what OpenGL renders into; larger than window size on HiDPI)
        void get_drawable_size(int& w, int& h) const { SDL_GL_GetDrawableSize(window, &w, &h); }
    private:
        SDL_Window* window{nullptr};
        SDL_GLContext gl_context{nullptr};
    };
}
