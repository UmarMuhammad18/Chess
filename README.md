# Chess

A cross-platform chess engine and GUI built from scratch in **C++20** using **SDL2** and **OpenGL 3.3 Core**. No chess libraries — bitboards, search, evaluation, and the windowed client are all hand-written.

Built as a learning and CV project.

---

## What you get

| Binary | What it is |
|---|---|
| `chess_engine` | Playable desktop GUI (human vs engine) |
| `chess_uci` | UCI engine for Arena, Cute Chess, etc. |
| `engine_test` | GoogleTest suite (perft, rules, search, notation) |

---

## Play the GUI

Click a piece, then a highlighted square. The engine replies on a background thread so the window stays responsive.

| Control | Action |
|---|---|
| Click piece / square | Select and move (legal dots shown) |
| **New Game** or `N` | Reset the board |
| **Undo** or `U` | Take back the last human move (and the engine reply) |
| **Resign** or `R` | Resign the current game |
| **White / Black** | Play as that colour (board flips when you play Black) |
| **0.5s / 1s / 2s / 5s** | Engine think time per move |

The side panel shows status, search depth / score / nodes, a multi-move PV, and a SAN move list.

---

## Engine features

- **Bitboards** — 12 piece/colour occupancies, 16-bit packed moves
- **FEN** — load any position
- **Legal movegen** — pawns, leapers, classical ray sliders, castling
- **Make / unmake** — full reversible state, including hash and repetition list
- **Null-move** — reversible null-move support for pruning
- **Draw detection** — threefold, 50-move, insufficient material
- **Zobrist + TT** — incremental hash, 32MB transposition table
- **Search** — iterative deepening, PVS, null-move, LMR, check extensions, killers, history, aspiration windows
- **Long PV** — principal variation reconstructed by walking the TT
- **Quiescence** — captures/promotions with SEE-style filtering
- **Evaluation** — material + PSTs, bishop pair, mobility, pawn structure, open files
- **PGN** — `moves_to_pgn` / `pgn_to_moves` helpers in notation
- **Time management** — `movetime` or `wtime/btime/winc/binc`
- **Notation** — UCI and SAN
- **UCI** — full protocol for GUIs

---

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./build/chess_engine
./build/chess_uci
ctest --test-dir build --output-on-failure
```

---

## Roadmap

- [x] Playable GUI
- [x] Iterative deepening with time management
- [x] Quiescence search
- [x] Piece-square tables
- [x] UCI protocol
- [x] Null-move / LMR / killers / history
- [x] Stronger evaluation
- [x] Longer PV from TT
- [x] PGN helpers in notation
- [ ] Magic bitboards (O(1) slider attacks)
- [ ] Under-promotion picker in the GUI
- [ ] Analysis mode + PGN export button
- [ ] Android / GLES build via the NDK

---

## License

MIT License — fork, learn from, and build on it.
