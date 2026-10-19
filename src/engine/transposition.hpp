#pragma once
#include "types.hpp"
#include <cstddef>
#include <vector>

namespace engine {
    // ALPHA = upper bound (fail-low), BETA = lower bound (fail-high)
    enum class HashFlag : uint8_t { NONE, EXACT, ALPHA, BETA };

    struct TTEntry {
        U64      key{0};
        Move     best_move{};
        int16_t  score{0};
        int8_t   depth{0};
        HashFlag flag{HashFlag::NONE};
        uint8_t  generation{0};
    };

    class TranspositionTable {
    public:
        explicit TranspositionTable(size_t mb_size);
        void resize(size_t mb_size);
        void clear();
        // Call once per search so entries from older searches are replaced first
        void new_search() { generation++; }

        // Copies the entry for key into out; returns false if not present
        bool probe(U64 key, TTEntry& out) const;
        void store(U64 key, int depth, int score, HashFlag flag, Move best_move);

        // Permille of sampled slots used by the current search (UCI "hashfull")
        int hashfull() const;
        size_t size() const { return table.size(); }
    private:
        std::vector<TTEntry> table;
        uint8_t generation{0};
    };

    // Mate scores are stored relative to the node, not the root, so they stay
    // correct when the same position is reached at a different ply
    inline int score_to_tt(int score, int ply) {
        if (score >= MATE_BOUND)  return score + ply;
        if (score <= -MATE_BOUND) return score - ply;
        return score;
    }
    inline int score_from_tt(int score, int ply) {
        if (score >= MATE_BOUND)  return score - ply;
        if (score <= -MATE_BOUND) return score + ply;
        return score;
    }
}
