#pragma once
#include "types.hpp"

/*
 * Precomputed attack tables. Leaper attacks (pawn, knight, king) are plain
 * lookups; slider attacks (bishop, rook, queen) use magic bitboards so each
 * query is a mask, a multiply, a shift and a table read.
 */
namespace engine::attacks {
    // Must be called once before any lookup (idempotent)
    void init();

    U64 pawn(Color c, Square sq);
    U64 knight(Square sq);
    U64 king(Square sq);
    U64 bishop(Square sq, U64 occupancy);
    U64 rook(Square sq, U64 occupancy);
    inline U64 queen(Square sq, U64 occupancy) { return bishop(sq, occupancy) | rook(sq, occupancy); }

    // Reference implementation used to build and verify the magic tables
    U64 bishop_slow(Square sq, U64 occupancy);
    U64 rook_slow(Square sq, U64 occupancy);
}
