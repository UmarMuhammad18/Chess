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
| Click piece / square | Select and move (legal dots shown); under-promotion picker for Q/R/B/N |
| **New Game** or `N` | Reset the board |
| **Undo** or `U` | Take back the last human move (and the engine reply) |
| **Resign** or `R` | Resign the current game |
| **White / Black** | Play as that colour (board flips when you play Black) |
| **0.5s / 1s / 2s / 5s** | Engine think time per move |
| **Analysis** | Toggle analysis mode (search without auto-playing) |
| **Export PGN** | Write `game.pgn` and print PGN to stdout |

The side panel shows status, search depth / score / nodes, a multi-move PV, and a SAN move list.

---

## Architecture

```
src/
├── engine/       Pure C++ (no OS, no SDL, no OpenGL)
├── uci/          Universal Chess Interface front-end
├── platform/     SDL2 windowing, input, timing
├── render/       OpenGL 3.3 shaders, textures, bitmap UI
└── game/         GUI: engine ↔ platform ↔ renderer
```

### Engine
| File | Responsibility |
|---|---|
| `types.hpp` | `U64`, `Square`, `Piece`, `Color`, 16-bit `Move`, `GameResult` |
| `utils.hpp` | `popcount`, `lsb`, `pop_lsb` via C++20 `<bit>` |
| `bitboard.hpp` | Inline `set_bit`, `clear_bit`, `get_bit` |
| `board.hpp/cpp` | FEN, make/unmake, null-move, history, draws |
| `movegen.hpp/cpp` | Leapers, **magic bitboards** for sliders, legal/noisy gen |
| `eval.hpp/cpp` | Material + PSTs + bishop pair, mobility, pawn structure, open files |
| `search.hpp/cpp` | ID, PVS, null-move, LMR, killers, history, aspiration, long PV from TT |
| `notation.hpp/cpp` | UCI / SAN / **PGN** export-import, game-result strings |
| `zobrist.hpp/cpp` | Incremental 64-bit hashing |
| `transposition.hpp/cpp` | 32MB TT with Exact / Alpha / Beta bounds |

---

## Engine features

- **Bitboards** — 12 piece/colour occupancies, 16-bit packed moves
- **FEN** — load any position
- **Legal movegen** — pawns, leapers, sliders, castling
- **Magic bitboards** — O(1) rook/bishop attacks after init
- **Make / unmake** — full reversible state, including hash and repetition list
- **Null-move** — reversible null-move support for pruning
- **Draw detection** — threefold, 50-move, insufficient material
- **Zobrist + TT** — incremental hash, 32MB transposition table
- **Search** — iterative deepening, PVS, null-move, LMR, check extensions, killers, history, aspiration windows
- **Long PV** — principal variation reconstructed by walking the TT
- **Quiescence** — captures/promotions with SEE-style filtering
- **Evaluation** — material + PSTs, bishop pair, mobility, pawn structure, open files
- **PGN** — export games to `game.pgn` / stdout; simple import helper
- **Time management** — `movetime` or `wtime/btime/winc/binc`
- **Notation** — UCI and SAN
- **UCI** — full protocol for GUIs

---

## Build

### Prerequisites

**Windows** (via [vcpkg](https://github.com/microsoft/vcpkg)):
```powershell
vcpkg install sdl2 glew
```

**macOS**:
```bash
brew install cmake sdl2 glew
```

**Linux**:
```bash
sudo apt install cmake libsdl2-dev libglew-dev
```

### Configure & build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Run

```bash
./build/chess_engine
./build/chess_uci
```

### Tests

```bash
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
- [x] Magic bitboards
- [x] Longer PV from TT
- [x] Under-promotion picker
- [x] Analysis mode + PGN export
- [ ] Android / GLES build via the NDK

---

## License

MIT License — fork, learn from, and build on it.
