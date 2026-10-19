/* Entry point for the UCI engine binary. Speaks UCI on stdin/stdout so the engine
   can be loaded into Arena, Cute Chess, BanksiaGUI, lichess-bot and similar tools. */
#include "uci/uci.hpp"
#include <iostream>

int main() {
    std::ios::sync_with_stdio(false);
    uci::UciEngine engine(std::cout);
    engine.loop(std::cin);
    return 0;
}
