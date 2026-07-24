#include "game/game.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    game::Game g;
    if (!g.init()) {
        std::cerr << "Failed to init game!" << std::endl;
        return -1;
    }
    g.loop();
    return 0;
}
