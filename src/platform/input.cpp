#include "input.hpp"

namespace platform {
    void Input::reset_frame() {
        mouse_just_pressed = false;
        mouse_just_released = false;
    }

    void Input::update(const SDL_Event& e) {
        switch (e.type) {
            case SDL_QUIT:
                quit = true;
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (e.button.button == SDL_BUTTON_LEFT) {
                    mouse_just_pressed = true;
                    mouse_down = true;
                    mouse_x = e.button.x;
                    mouse_y = e.button.y;
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if (e.button.button == SDL_BUTTON_LEFT) {
                    mouse_just_released = true;
                    mouse_down = false;
                    mouse_x = e.button.x;
                    mouse_y = e.button.y;
                }
                break;
            case SDL_MOUSEMOTION:
                mouse_x = e.motion.x;
                mouse_y = e.motion.y;
                break;
            default:
                break;
        }
    }
}
