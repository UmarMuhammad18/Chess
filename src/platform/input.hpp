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
        bool key_new() const { return key_new_game; }
        bool key_undo() const { return key_undo_move; }
        bool key_resign() const { return key_resign_game; }
    private:
        bool quit{false};
        bool mouse_just_pressed{false};
        int mouse_x{0};
        int mouse_y{0};
        bool key_new_game{false};
        bool key_undo_move{false};
        bool key_resign_game{false};
    };
}
