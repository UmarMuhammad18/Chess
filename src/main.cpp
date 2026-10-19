/* Entry point for the Chess Engine application. Sets up the Game object and runs the main loop. */
#define SDL_MAIN_HANDLED
#include "game/game.hpp"
#include <SDL2/SDL.h>
#include <iostream>

int main(int, char*[]) {
    SDL_SetMainReady();
    // Create the game instance
    game::Game g;
    // Initialise the game; abort on failure
    if (!g.init()) {
        std::cerr << "Failed to init game!" << std::endl;
        return -1;
    }
    // Run the main game loop
    g.loop();
    return 0;
}
