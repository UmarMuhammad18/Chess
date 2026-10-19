#pragma once
#include "../engine/board.hpp"
#include "../engine/search.hpp"
#include <atomic>
#include <iosfwd>
#include <mutex>
#include <string>
#include <thread>

/*
 * Universal Chess Interface front end. Reads commands line by line and runs
 * "go" on a worker thread so "stop", "isready" and "quit" stay responsive.
 */
namespace uci {
    class UciEngine {
    public:
        explicit UciEngine(std::ostream& out);
        ~UciEngine();

        // Reads commands until "quit" or end of input
        void loop(std::istream& in);
        // Handles one command line; returns false on "quit"
        bool handle(const std::string& line);
        // Blocks until a running search has printed its bestmove
        void wait();

    private:
        void cmd_position(std::istringstream& args);
        void cmd_go(std::istringstream& args);
        void cmd_setoption(std::istringstream& args);
        void cmd_perft(std::istringstream& args);
        void stop_search();
        void send(const std::string& line);

        std::ostream& out;
        std::mutex out_mutex;
        engine::Board board;
        engine::Search search;
        std::thread worker;
        std::atomic<bool> searching{false};
    };
}
