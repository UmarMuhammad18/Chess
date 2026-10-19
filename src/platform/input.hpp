#pragma once
#include <SDL2/SDL.h>

namespace platform {
    class Input {
    public:
        void update(const SDL_Event& e);
        void reset_frame();
        bool quit_requested() const { return quit; }

        bool is_mouse_just_pressed() const { return mouse_just_pressed; }
        bool is_mouse_just_released() const { return mouse_just_released; }
        bool is_mouse_down() const { return mouse_down; }
        // Position in window (screen) coordinates
        void get_mouse_pos(int& x, int& y) const { x = mouse_x; y = mouse_y; }
    private:
        bool quit{false};
        bool mouse_just_pressed{false};
        bool mouse_just_released{false};
        bool mouse_down{false};
        int mouse_x{0};
        int mouse_y{0};
    };
}
