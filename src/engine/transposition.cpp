#include "transposition.hpp"

namespace engine {
    TranspositionTable::TranspositionTable(size_t mb_size) {
        size_t num_entries = (mb_size * 1024 * 1024) / sizeof(TTEntry);
        table.resize(num_entries);
        clear();
    }

    void TranspositionTable::clear() {
        for (auto& entry : table) {
            entry.key = 0;
            entry.depth = -1;
        }
    }

    void TranspositionTable::store(U64 key, int depth, int score, HashFlag flag, Move best_move) {
        size_t index = key % table.size();
        // Always replace strategy for scaffolding
        table[index].key = key;
        table[index].depth = depth;
        table[index].score = score;
        table[index].flag = flag;
        table[index].best_move = best_move;
    }

    bool TranspositionTable::probe(U64 key, int depth, int alpha, int beta, int& return_score, Move& best_move) {
        size_t index = key % table.size();
        TTEntry& entry = table[index];
        
        if (entry.key == key) {
            best_move = entry.best_move;
            if (entry.depth >= depth) {
                if (entry.flag == HashFlag::EXACT) {
                    return_score = entry.score;
                    return true;
                }
                if (entry.flag == HashFlag::ALPHA && entry.score <= alpha) {
                    return_score = alpha;
                    return true;
                }
                if (entry.flag == HashFlag::BETA && entry.score >= beta) {
                    return_score = beta;
                    return true;
                }
            }
        }
        return false;
    }
}
