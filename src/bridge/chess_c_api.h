#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/** Opaque engine handle (one game / search session). */
typedef struct ChessEngine ChessEngine;

/** Create engine and load starting position. Caller must chess_destroy(). */
ChessEngine* chess_create(void);

/** Free engine. */
void chess_destroy(ChessEngine* e);

/** Load FEN. Returns 1 on success, 0 on failure. */
int chess_load_fen(ChessEngine* e, const char* fen);

/** Side to move: 0 = white, 1 = black. */
int chess_side_to_move(ChessEngine* e);

/**
 * Search for movetime_ms milliseconds.
 * Writes UCI move into out_uci (at least 8 bytes, e.g. "e2e4" or "e7e8q").
 * Returns score in centipawns (side to move POV), or INT_MIN on failure.
 */
int chess_search(ChessEngine* e, int movetime_ms, char* out_uci, int out_len);

/**
 * Apply a UCI move (e.g. "e2e4"). Returns 1 if legal and applied, 0 otherwise.
 */
int chess_make_uci(ChessEngine* e, const char* uci);

/**
 * Fill out_uci with a space-separated list of legal UCI moves.
 * Returns number of moves, or -1 if buffer too small.
 */
int chess_legal_moves(ChessEngine* e, char* out_uci, int out_len);

/** Game result: 0 ongoing, 1 white mates, 2 black mates, 3 draw/stalemate/other. */
int chess_result(ChessEngine* e);

#ifdef __cplusplus
}
#endif
