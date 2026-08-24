#include "input.hpp"

namespace platform {
    void Input::reset_frame() {
        mouse_just_pressed = false;
        key_new_game = false;
        key_undo_move = false;
        key_resign_game = false;
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
        } else if (e.type == SDL_MOUSEMOTION) {
            mouse_x = e.motion.x;
            mouse_y = e.motion.y;
        } else if (e.type == SDL_KEYDOWN && e.key.repeat == 0) {
            if (e.key.keysym.sym == SDLK_n) key_new_game = true;
            if (e.key.keysym.sym == SDLK_u) key_undo_move = true;
            if (e.key.keysym.sym == SDLK_r) key_resign_game = true;
        }
    }
}
