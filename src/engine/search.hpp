#pragma once
#include "board.hpp"
#include "movegen.hpp"
#include "transposition.hpp"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <vector>

namespace engine {
    // What the caller allows the search to spend. Unset values (-1 / 0) mean "no limit".
    struct SearchLimits {
        int      depth      = MAX_PLY - 1;
        int64_t  movetime   = -1;        // exact milliseconds for this move
        int64_t  time[2]    = {-1, -1};  // remaining clock per color (ms)
        int64_t  inc[2]     = {0, 0};    // increment per color (ms)
        int      movestogo  = 0;
        uint64_t nodes      = 0;
        bool     infinite   = false;
    };

    // Snapshot reported after each completed iteration
    struct SearchInfo {
        int      depth    = 0;
        int      seldepth = 0;
        int      score    = 0;   // centipawns from the side to move's perspective
        uint64_t nodes    = 0;
        int64_t  time_ms  = 0;
        int      hashfull = 0;
        std::vector<Move> pv;

        bool is_mate() const { return score >= MATE_BOUND || score <= -MATE_BOUND; }
        // Signed moves-to-mate (positive: side to move mates)
        int mate_in() const { return score > 0 ? (MATE_SCORE - score + 1) / 2 : -(MATE_SCORE + score) / 2; }
    };

    class Search {
    public:
        using InfoCallback = std::function<void(const SearchInfo&)>;

        explicit Search(size_t tt_mb = 16);

        // Iterative deepening search. Returns a null move only if there are no legal moves.
        // Safe to call request_stop() from another thread while this runs.
        Move think(Board& board, const SearchLimits& limits, const InfoCallback& on_info = {});
        // Fixed-depth convenience wrapper
        Move search_best_move(Board& board, int depth);

        void request_stop() { stop_flag.store(true, std::memory_order_relaxed); }
        bool stop_requested() const { return stop_flag.load(std::memory_order_relaxed); }
        void new_game();
        void set_hash_size(size_t mb) { tt.resize(mb); }
        const SearchInfo& last_info() const { return info; }

    private:
        int  negamax(Board& board, int depth, int ply, int alpha, int beta, bool allow_null);
        int  quiescence(Board& board, int ply, int alpha, int beta);
        void score_moves(const Board& board, const MoveList& moves, int* scores, Move tt_move, int ply) const;
        void check_limits();
        int64_t elapsed_ms() const;
        void update_pv(int ply, Move m);

        TranspositionTable tt;
        std::atomic<bool> stop_flag{false};

        Move killers[MAX_PLY][2];
        int  history[2][64][64];
        Move pv_table[MAX_PLY][MAX_PLY];
        int  pv_length[MAX_PLY];

        uint64_t nodes{0};
        uint64_t node_limit{0};
        int      seldepth{0};
        int64_t  hard_limit_ms{-1};
        std::chrono::steady_clock::time_point start_time;
        SearchInfo info;
    };
}
