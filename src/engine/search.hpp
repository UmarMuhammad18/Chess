#pragma once
#include "board.hpp"
#include "movegen.hpp"
#include "transposition.hpp"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <array>
#include <vector>

namespace engine {
    struct SearchLimits {
        int max_depth = 64;
        int movetime_ms = 0;
        int wtime_ms = 0;
        int btime_ms = 0;
        int winc_ms = 0;
        int binc_ms = 0;
        bool use_clock = true;
    };

    struct SearchResult {
        Move best{};
        int score = 0;
        int depth = 0;
        std::uint64_t nodes = 0;
        std::vector<Move> pv;
    };

    class Search {
    public:
        Search() : tt(32) {}   // 32 MB TT
        SearchResult search(Board& board, const SearchLimits& limits);
        Move search_best_move(Board& board, int depth);
        void request_stop() { stop.store(true); }
        void clear_stop() { stop.store(false); }
        TranspositionTable tt;

    private:
        static constexpr int MAX_PLY = 128;
        static constexpr int MATE = 30000;
        static constexpr int INF  = 32000;

        std::atomic<bool> stop{false};
        std::uint64_t nodes = 0;
        std::chrono::steady_clock::time_point t_end{};
        bool timed = false;

        // Killer moves: 2 slots per ply
        std::array<std::array<Move, 2>, MAX_PLY> killers{};

        // History heuristic: [color][from][to]
        std::array<std::array<std::array<int, 64>, 64>, 2> history{};

        bool time_up();
        void clear_killers();
        void clear_history();

        int score_move(const Board& board, const Move& move, Move tt_move, int ply);
        int quiesce(Board& board, int alpha, int beta, int ply);
        int negamax(Board& board, int depth, int alpha, int beta, int ply, bool do_null);
    };
}
