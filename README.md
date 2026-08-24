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

The side panel shows status (check, mate, draw, “AI thinking”), search depth / score / nodes, and a SAN move list. Promotions in the GUI always become a queen.

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
| `board.hpp/cpp` | FEN, make/unmake, history, repetition / 50-move / material draws |
| `movegen.hpp/cpp` | Leaper tables, classical rays, legal (and noisy) generation |
| `eval.hpp/cpp` | Material + piece-square tables, midgame/endgame king |
| `search.hpp/cpp` | Iterative deepening, alpha-beta, quiescence, time management |
| `notation.hpp/cpp` | UCI / SAN, check/mate suffixes, game-result strings |
| `zobrist.hpp/cpp` | Incremental 64-bit hashing |
| `transposition.hpp/cpp` | 16MB TT with Exact / Alpha / Beta bounds |

### GUI
| File | Responsibility |
|---|---|
| `platform/` | SDL2 window (OpenGL 3.3 Core, MSAA 4x, VSync), mouse + keys |
| `render/` | NDC textured quads, sprite atlas, 5×7 bitmap font, tints / dots |
| `game/` | Selection, legal-move hints, AI worker thread, panel buttons |

---

## Engine features

- **Bitboards** — 12 piece/colour occupancies, 16-bit packed moves
- **FEN** — load any position
- **Legal movegen** — pawns (push, double, capture, EP, promo ×4), leapers, sliders, castling
- **Make / unmake** — full reversible state, including hash and repetition list
- **Draw detection** — threefold repetition, 50-move rule, insufficient material
- **Zobrist + TT** — incremental hash, 16MB transposition table
- **Search** — negamax alpha-beta, MVV-LVA + TT move ordering, iterative deepening
- **Quiescence** — captures and promotions; full legal moves while in check
- **Evaluation** — material (N=320, B=330) plus PSTs; king uses midgame vs endgame tables
- **Time management** — `movetime`, or `wtime/btime/winc/binc` (remaining/30 + increment/2)
- **Notation** — UCI (`e2e4`, `e7e8q`) and SAN (`Nf3`, `O-O`, `a8=Q#`)
- **UCI** — `uci`, `isready`, `ucinewgame`, `position startpos\|fen … moves …`, `go depth\|movetime\|wtime/btime/winc/binc\|infinite`, `stop`, `quit`

---

## Tech stack

| Layer | Technology |
|---|---|
| Language | C++20 |
| Build | CMake 3.16+ |
| Windowing | SDL2 |
| Rendering | OpenGL 3.3 Core |
| Extension loader | GLEW |
| Images | stb_image (single-header) |
| Testing | GoogleTest (FetchContent) |

Desktop only for now (Linux, macOS, Windows). There is no GLES / Android build yet.

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
# GUI
./build/chess_engine              # Linux / macOS
.\build\Release\chess_engine.exe  # Windows

# UCI (pipe, or point Arena / Cute Chess at this binary)
./build/chess_uci
```

### Tests

```bash
ctest --test-dir build --output-on-failure
```

Covered today:

- FEN parsing and starting-position legality (20 moves)
- Perft: startpos 1–4 (20 / 400 / 8,902 / 197,281) and Kiwipete 1–3 (48 / 2,039 / 97,862)
- Make/unmake hash reversibility; Zobrist matches a loaded FEN after `e2e4`
- Promotion captures, 7th-rank pushes (not promotions), black castling-through-check
- Stalemate vs checkmate scoring; search prefers mate (`#` in SAN)
- Piece-square tables (centre pawn, advanced pawn)
- UCI / SAN (`e2e4`, `O-O`, `a7a8q`)
- Threefold repetition, mate and stalemate results
- Timed iterative-deepening search returns a legal move

---

## Project layout

```
Chess/
├── CMakeLists.txt
├── README.md
├── .github/workflows/build.yml    Linux / macOS / Windows CI
├── assets/
│   ├── shaders/                   OpenGL 3.3 vertex + fragment
│   └── textures/                  Board and piece atlas
├── src/
│   ├── main.cpp                   GUI entry
│   ├── uci/uci.cpp                UCI entry
│   ├── vendor/stb_image.h
│   ├── engine/
│   ├── platform/
│   ├── render/
│   └── game/
└── test/
    └── engine_test.cpp
```

---

## Roadmap

- [x] Playable GUI (legal hints, undo, side to play, timed AI thread)
- [x] Iterative deepening with time management
- [x] Quiescence search
- [x] Piece-square tables
- [x] UCI protocol (`chess_uci`)
- [ ] Magic bitboards (O(1) slider attacks)
- [ ] Null-move pruning / late-move reductions
- [ ] Principal-variation search and a longer PV from the TT
- [ ] Under-promotion picker in the GUI
- [ ] Android / GLES build via the NDK

---

## License

MIT License — fork, learn from, and build on it.
