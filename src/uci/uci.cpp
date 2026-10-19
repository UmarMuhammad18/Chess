#include "uci.hpp"
#include "../engine/movegen.hpp"
#include "../engine/notation.hpp"
#include "../engine/perft.hpp"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <iostream>
#include <sstream>

namespace uci {
    namespace {
        constexpr int DEFAULT_HASH_MB = 16;

        std::string format_score(const engine::SearchInfo& info) {
            if (info.is_mate()) return "mate " + std::to_string(info.mate_in());
            return "cp " + std::to_string(info.score);
        }
    }

    UciEngine::UciEngine(std::ostream& o) : out(o), search(DEFAULT_HASH_MB) {
        engine::MoveGen::init();
        board.load_fen(engine::START_FEN);
    }

    UciEngine::~UciEngine() {
        stop_search();
    }

    void UciEngine::send(const std::string& line) {
        std::lock_guard<std::mutex> lock(out_mutex);
        out << line << std::endl;
    }

    void UciEngine::loop(std::istream& in) {
        std::string line;
        while (std::getline(in, line)) {
            if (!handle(line)) break;
        }
        stop_search();
    }

    void UciEngine::wait() {
        if (worker.joinable()) worker.join();
    }

    void UciEngine::stop_search() {
        if (!worker.joinable()) return;
        // think() clears the stop flag when it starts, so keep asking until the worker is done
        while (searching.load()) {
            search.request_stop();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        worker.join();
    }

    bool UciEngine::handle(const std::string& line) {
        std::istringstream args(line);
        std::string cmd;
        args >> cmd;

        if (cmd == "uci") {
            send("id name ChessEngine 0.2");
            send("id author Umar Muhammad");
            send("option name Hash type spin default " + std::to_string(DEFAULT_HASH_MB) + " min 1 max 1024");
            send("uciok");
        } else if (cmd == "isready") {
            send("readyok");
        } else if (cmd == "ucinewgame") {
            stop_search();
            search.new_game();
            board.load_fen(engine::START_FEN);
        } else if (cmd == "setoption") {
            cmd_setoption(args);
        } else if (cmd == "position") {
            stop_search();
            cmd_position(args);
        } else if (cmd == "go") {
            stop_search();
            cmd_go(args);
        } else if (cmd == "stop") {
            stop_search();
        } else if (cmd == "quit") {
            stop_search();
            return false;
        } else if (cmd == "d") {
            std::lock_guard<std::mutex> lock(out_mutex);
            out << "FEN: " << board.get_fen() << std::endl;
        } else if (cmd == "perft") {
            stop_search();
            cmd_perft(args);
        } else if (!cmd.empty()) {
            send("info string unknown command: " + cmd);
        }
        return true;
    }

    void UciEngine::cmd_position(std::istringstream& args) {
        std::string token;
        args >> token;
        if (token == "startpos") {
            board.load_fen(engine::START_FEN);
            args >> token; // optional "moves"
        } else if (token == "fen") {
            std::string fen, part;
            while (args >> part && part != "moves") fen += part + " ";
            if (!board.load_fen(fen)) {
                send("info string invalid fen: " + fen);
                return;
            }
            token = part;
        } else {
            return;
        }

        if (token != "moves") return;
        std::string move_text;
        while (args >> move_text) {
            engine::Move m = engine::parse_uci_move(board, move_text);
            if (m.is_null()) {
                send("info string illegal move: " + move_text);
                return;
            }
            board.make_move(m);
        }
    }

    void UciEngine::cmd_go(std::istringstream& args) {
        engine::SearchLimits limits;
        int perft_depth = 0;
        std::string token;
        while (args >> token) {
            if      (token == "depth")     args >> limits.depth;
            else if (token == "movetime")  args >> limits.movetime;
            else if (token == "wtime")     args >> limits.time[0];
            else if (token == "btime")     args >> limits.time[1];
            else if (token == "winc")      args >> limits.inc[0];
            else if (token == "binc")      args >> limits.inc[1];
            else if (token == "movestogo") args >> limits.movestogo;
            else if (token == "nodes")     args >> limits.nodes;
            else if (token == "infinite")  limits.infinite = true;
            else if (token == "perft")     args >> perft_depth;
        }
        if (perft_depth > 0) {
            std::istringstream depth_args(std::to_string(perft_depth));
            cmd_perft(depth_args);
            return;
        }

        engine::Board root = board;
        searching = true;
        worker = std::thread([this, root, limits]() mutable {
            engine::Move best = search.think(root, limits, [this](const engine::SearchInfo& info) {
                std::ostringstream line;
                int64_t nps = info.time_ms > 0 ? static_cast<int64_t>(info.nodes * 1000 / info.time_ms) : 0;
                line << "info depth " << info.depth << " seldepth " << info.seldepth
                     << " score " << format_score(info) << " nodes " << info.nodes
                     << " nps " << nps << " time " << info.time_ms << " hashfull " << info.hashfull << " pv";
                for (engine::Move m : info.pv) line << ' ' << engine::move_to_uci(m);
                send(line.str());
            });
            // In infinite mode the GUI expects bestmove only after it sends "stop"
            while (limits.infinite && !search.stop_requested())
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            send("bestmove " + engine::move_to_uci(best));
            searching = false;
        });
    }

    void UciEngine::cmd_setoption(std::istringstream& args) {
        std::string token, name, value;
        args >> token; // "name"
        while (args >> token && token != "value") name += (name.empty() ? "" : " ") + token;
        args >> value;
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        if (name == "hash") {
            stop_search();
            try {
                search.set_hash_size(static_cast<size_t>(std::clamp(std::stoi(value), 1, 1024)));
            } catch (const std::exception&) {
                send("info string invalid hash size: " + value);
            }
        }
    }

    void UciEngine::cmd_perft(std::istringstream& args) {
        int depth = 1;
        args >> depth;
        auto start = std::chrono::steady_clock::now();
        uint64_t total = 0;
        for (const engine::Move& m : engine::MoveGen::generate_legal_moves(board)) {
            board.make_move(m);
            uint64_t n = depth > 1 ? engine::perft(board, depth - 1) : 1;
            board.unmake_move(m);
            total += n;
            send(engine::move_to_uci(m) + ": " + std::to_string(n));
        }
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
        send("\nNodes searched: " + std::to_string(total) + " (" + std::to_string(ms) + " ms)");
    }
}
