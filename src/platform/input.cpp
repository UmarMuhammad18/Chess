#include "input.hpp"

namespace platform {
    void Input::reset_frame() {
        mouse_just_pressed = false;
    }

    void Input::update(const SDL_Event& e) {
        if (e.type == SDL_QUIT) {
            quit = true;
        } else if (e.type == SDL_MOUSEBUTTONDOWN) {
            if (e.button.button == SDL_BUTTON_LEFT) {
                mouse_just_pressed = true;
                mouse_x = e.button.x;
                mouse_y = e.button.y;
            }
        }
    }
}
