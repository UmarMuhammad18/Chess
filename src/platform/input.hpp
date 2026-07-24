#pragma once
#include <SDL2/SDL.h>

namespace platform {
    class Input {
    public:
        void update(const SDL_Event& e);
        void reset_frame();
        bool quit_requested() const { return quit; }
        
        bool is_mouse_just_pressed() const { return mouse_just_pressed; }
        void get_mouse_pos(int& x, int& y) const { x = mouse_x; y = mouse_y; }
    private:
        bool quit{false};
        bool mouse_just_pressed{false};
        int mouse_x{0};
        int mouse_y{0};
    };
}
