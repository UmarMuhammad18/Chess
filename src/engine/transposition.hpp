#pragma once
#include "types.hpp"
#include <cstddef>
#include <vector>

namespace engine {
    enum class HashFlag { EXACT, ALPHA, BETA };

    struct TTEntry {
        U64 key;
        int depth;
        int score;
        HashFlag flag;
        Move best_move;
    };

    class TranspositionTable {
    public:
        TranspositionTable(size_t mb_size);
        void clear();
        void store(U64 key, int depth, int score, HashFlag flag, Move best_move);
        bool probe(U64 key, int depth, int alpha, int beta, int& return_score, Move& best_move);
        
        size_t size() const { return table.size(); }
    private:
        std::vector<TTEntry> table;
    };
}
