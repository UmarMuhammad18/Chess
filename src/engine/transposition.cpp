#include "transposition.hpp"
#include <algorithm>

namespace engine {
    TranspositionTable::TranspositionTable(size_t mb_size) {
        resize(mb_size);
    }

    void TranspositionTable::resize(size_t mb_size) {
        size_t num_entries = std::max<size_t>(1, (std::max<size_t>(1, mb_size) * 1024 * 1024) / sizeof(TTEntry));
        table.assign(num_entries, TTEntry{});
        generation = 0;
    }

    void TranspositionTable::clear() {
        std::fill(table.begin(), table.end(), TTEntry{});
        generation = 0;
    }

    bool TranspositionTable::probe(U64 key, TTEntry& out) const {
        const TTEntry& entry = table[key % table.size()];
        if (entry.flag == HashFlag::NONE || entry.key != key) return false;
        out = entry;
        return true;
    }

    void TranspositionTable::store(U64 key, int depth, int score, HashFlag flag, Move best_move) {
        TTEntry& entry = table[key % table.size()];

        // Depth-preferred replacement, but always replace stale entries from
        // earlier searches and never let a shallow result evict a deep one
        bool same = entry.key == key;
        if (!same && entry.flag != HashFlag::NONE && entry.generation == generation &&
            depth < entry.depth && flag != HashFlag::EXACT) {
            return;
        }
        if (same && depth < entry.depth - 2 && flag != HashFlag::EXACT) return;

        // Keep the old best move if this search didn't produce one
        if (!best_move.is_null() || !same) entry.best_move = best_move;
        entry.key        = key;
        entry.score      = static_cast<int16_t>(score);
        entry.depth      = static_cast<int8_t>(std::clamp(depth, -128, 127));
        entry.flag       = flag;
        entry.generation = generation;
    }

    int TranspositionTable::hashfull() const {
        size_t sample = std::min<size_t>(1000, table.size());
        int used = 0;
        for (size_t i = 0; i < sample; i++) {
            if (table[i].flag != HashFlag::NONE && table[i].generation == generation) used++;
        }
        return static_cast<int>(used * 1000 / sample);
    }
}
