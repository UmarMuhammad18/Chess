/* Entry point for the Chess Engine application. Sets up the Game object and runs the main loop. */
#include "game/game.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
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
