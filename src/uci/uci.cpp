#include "engine/board.hpp"
#include "engine/movegen.hpp"
#include "engine/notation.hpp"
#include "engine/search.hpp"
#include "engine/zobrist.hpp"
#include <atomic>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace engine;

static const char* START_FEN =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

static std::vector<std::string> split(const std::string& line) {
    std::istringstream iss(line);
    std::vector<std::string> tok;
    std::string t;
    while (iss >> t) tok.push_back(t);
    return tok;
}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    Zobrist::init();
    MoveGen::init();

    Board board;
    board.load_fen(START_FEN);
    Search searcher;
    std::thread worker;
    std::atomic<bool> searching{false};

    auto wait_search = [&]() {
        if (worker.joinable()) worker.join();
        searching = false;
    };

    auto stop_search = [&]() {
        searcher.request_stop();
        wait_search();
    };

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        auto tok = split(line);
        if (tok.empty()) continue;
        const std::string& cmd = tok[0];

        if (cmd == "uci") {
            std::cout << "id name Chess\n";
            std::cout << "id author Umar Muhammad\n";
            std::cout << "uciok\n";
        } else if (cmd == "isready") {
            wait_search();
            std::cout << "readyok\n";
        } else if (cmd == "ucinewgame") {
            stop_search();
            board.load_fen(START_FEN);
            searcher.tt.clear();
        } else if (cmd == "position") {
            stop_search();
            size_t i = 1;
            if (i < tok.size() && tok[i] == "startpos") {
                board.load_fen(START_FEN);
                i++;
            } else if (i < tok.size() && tok[i] == "fen") {
                i++;
                std::string fen;
                int parts = 0;
                while (i < tok.size() && tok[i] != "moves" && parts < 6) {
                    if (!fen.empty()) fen += ' ';
                    fen += tok[i++];
                    parts++;
                }
                board.load_fen(fen);
            }
            if (i < tok.size() && tok[i] == "moves") {
                i++;
                while (i < tok.size()) {
                    Move m = move_from_uci(board, tok[i++]);
                    if (m.data == 0) break;
                    board.make_move(m);
                }
            }
        } else if (cmd == "go") {
            stop_search();
            SearchLimits limits;
            limits.movetime_ms = 0;
            limits.max_depth = 64;
            limits.use_clock = true;
            bool got_depth = false;
            bool got_time = false;
            bool infinite = false;
            for (size_t i = 1; i < tok.size(); i++) {
                auto next_int = [&](int& dst) {
                    if (i + 1 < tok.size()) dst = std::stoi(tok[++i]);
                };
                if (tok[i] == "depth") { next_int(limits.max_depth); got_depth = true; }
                else if (tok[i] == "movetime") { next_int(limits.movetime_ms); got_time = true; }
                else if (tok[i] == "wtime") { next_int(limits.wtime_ms); got_time = true; }
                else if (tok[i] == "btime") { next_int(limits.btime_ms); got_time = true; }
                else if (tok[i] == "winc") { next_int(limits.winc_ms); got_time = true; }
                else if (tok[i] == "binc") { next_int(limits.binc_ms); got_time = true; }
                else if (tok[i] == "infinite") infinite = true;
            }
            if (infinite) {
                limits.use_clock = false;
                limits.max_depth = 64;
            } else if (got_time) {
                limits.use_clock = true;
            } else if (got_depth) {
                limits.use_clock = false;
            } else {
                limits.use_clock = true;
                limits.movetime_ms = 1000;
            }

            searching = true;
            worker = std::thread([&searcher, &board, &searching, limits]() {
                SearchResult r = searcher.search(board, limits);
                std::cout << "info depth " << r.depth
                          << " score cp " << r.score
                          << " nodes " << r.nodes
                          << " pv";
                for (const auto& m : r.pv) std::cout << ' ' << move_to_uci(m);
                std::cout << "\nbestmove " << move_to_uci(r.best) << "\n";
                std::cout.flush();
                searching = false;
            });
        } else if (cmd == "stop") {
            searcher.request_stop();
            wait_search();
        } else if (cmd == "quit") {
            stop_search();
            break;
        }
        std::cout.flush();
    }
    stop_search();
    return 0;
}
