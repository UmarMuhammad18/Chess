# Chess Engine

A cross-platform chess engine and GUI built from scratch in **C++20** using **SDL2**, **OpenGL 3.3 Core** and **Dear ImGui**, with a hand-written search stack and a **UCI** front end so the engine can play in any chess GUI.

Built as a learning and CV project — every engine layer was implemented by hand with no chess library dependencies.

---

## Architecture

The project is split into isolated layers. The engine and UCI layer are pure C++ and build without SDL or OpenGL.

```
src/
├── engine/       ← Pure C++ (no OS, no SDL, no OpenGL)
├── uci/          ← UCI protocol front end (chess_uci binary)
├── platform/     ← SDL2 windowing, input, timing
├── render/       ← OpenGL shader pipeline, textures, mesh
└── game/         ← Integration: engine ↔ platform ↔ renderer ↔ Dear ImGui
```

CMake targets:

| Target | Contents |
|---|---|
| `chess_core` | Static library: engine + UCI handler. Linked by everything else |
| `chess_uci` | Console UCI engine for Arena, Cute Chess, BanksiaGUI, lichess-bot |
| `chess_engine` | Desktop GUI (optional, `-DCHESS_BUILD_GUI=OFF` to skip) |
| `engine_test` | GoogleTest suite (perft, rules, search, UCI) |

### Engine Layer
| File | Responsibility |
|---|---|
| `types.hpp` | `U64`, `Square`, `Piece`, `Color`, 16-bit `Move`, fixed-capacity `MoveList`, score constants |
| `attacks.hpp/cpp` | Leaper tables and **magic bitboards** for sliders (magics generated deterministically at startup) |
| `board.hpp/cpp` | Bitboards + mailbox, FEN load/save with validation, make/unmake, null move, repetition / 50-move / material draws |
| `movegen.hpp/cpp` | Pseudo-legal, captures-only and legal move generation |
| `eval.hpp/cpp` | Tapered PeSTO evaluation (material + piece-square tables, middlegame/endgame blend) |
| `search.hpp/cpp` | Iterative deepening PVS with quiescence, TT, null-move, LMR, reverse futility, killers, history, time management |
| `transposition.hpp/cpp` | Depth-preferred hash table with generation aging and ply-adjusted mate scores |
| `zobrist.hpp/cpp` | 64-bit Zobrist hashing, incremental XOR updates |
| `notation.hpp/cpp` | UCI move strings and Standard Algebraic Notation (SAN) |
| `perft.hpp` | Perft node counter shared by tests and the UCI `perft` command |

### Platform / Render / Game
| File | Responsibility |
|---|---|
| `sdl_context` | SDL2 window, OpenGL 3.3 Core context (MSAA, VSync, HiDPI aware) |
| `app`, `input` | Event loop, mouse press/drag/release, ImGui event forwarding |
| `shader`, `texture`, `mesh` | Shader compile/link with cached uniforms, PNG/JPEG loading, unit quad |
| `renderer` | Pixel-space sprite renderer: board, chroma-keyed piece atlas, highlights, move dots |
| `game` | Game state, threaded engine, drag-and-drop, promotion picker, Dear ImGui side panel |

---

## Features

**Engine**
- **Magic Bitboards** — O(1) slider attacks; verified against ray-walking in tests
- **Full Legal Move Generation** — perft-verified on the standard CPW positions (startpos to depth 6: 119,060,324 nodes)
- **Iterative Deepening + Principal Variation Search** (negamax alpha-beta with null-window re-searches)
- **Quiescence Search** — captures and promotions (and all evasions when in check) with delta pruning
- **Transposition Table** — 64 MB in the GUI, configurable via UCI `Hash`
- **Pruning & Reductions** — null-move pruning, late move reductions, reverse futility pruning, mate distance pruning, check extension
- **Move Ordering** — TT move, MVV-LVA captures, two killer moves per ply, history heuristic
- **PeSTO Evaluation** — tapered middlegame/endgame piece-square tables
- **Draw Detection** — threefold repetition, 50-move rule, insufficient material, stalemate
- **Time Management** — clock + increment, `movetime`, depth/node limits, `infinite`

**GUI**
- Drag-and-drop or click-to-move, legal-move hints, last-move and check highlighting
- Engine runs on a worker thread, so the window never freezes
- Human vs engine (either colour), engine vs engine, or two humans
- Promotion picker, undo, flip board, pause, "move now"
- Live analysis: evaluation bar, depth, nodes/s, principal variation in SAN
- Move list in SAN, FEN load/copy, PGN export to clipboard
- Resizable window; renders at native resolution on HiDPI displays

---

## Tech Stack

| Layer | Technology |
|---|---|
| Language | C++20 |
| Build | CMake 3.20+, vcpkg manifest (Windows) |
| Windowing | SDL2 |
| Rendering | OpenGL 3.3 Core |
| UI | Dear ImGui 1.91 (FetchContent) |
| Extension Loader | GLEW |
| Image Loading | stb_image (single-header) |
| Testing | GoogleTest (FetchContent), ASan/UBSan in CI |

---

## Build Instructions

### Prerequisites

**Windows** — dependencies come from `vcpkg.json` automatically when you pass the vcpkg toolchain:
```powershell
cmake -B build -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
```

**macOS**:
```bash
brew install cmake sdl2 glew
```

**Linux**:
```bash
sudo apt install cmake libsdl2-dev libglew-dev
```

Engine/UCI/tests only (no SDL, OpenGL or GLEW needed):
```bash
cmake -B build -DCHESS_BUILD_GUI=OFF
```

### Configure & Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Run the GUI
./build/chess_engine                    # Linux/macOS
.\build\Release\chess_engine.exe        # Windows (Visual Studio generator)
```

| Option | Default | Purpose |
|---|---|---|
| `CHESS_BUILD_GUI` | `ON` | Build the SDL2/OpenGL GUI |
| `CHESS_BUILD_TESTS` | `ON` | Build the GoogleTest suite |
| `CHESS_ENABLE_SANITIZERS` | `OFF` | AddressSanitizer + UBSan (GCC/Clang) |
| `CHESS_WARNINGS_AS_ERRORS` | `OFF` | `-Werror` / `/WX` |

### Run Tests

```bash
ctest --test-dir build --output-on-failure          # everything
ctest --test-dir build --output-on-failure -LE slow # skip deep perft
```

The suite covers perft on five standard positions, FEN round-trips and validation, magic vs. reference attacks, SAN/UCI notation, repetition and draw rules, tactical search positions (mates, hanging pieces, horizon effect), time limits, and the UCI protocol.

---

## Using the UCI Engine

`chess_uci` speaks the Universal Chess Interface on stdin/stdout. Add it as an engine in Arena, Cute Chess or BanksiaGUI, or use it with [lichess-bot](https://github.com/lichess-bot-devs/lichess-bot).

```
$ ./build/chess_uci
uci
id name ChessEngine 0.2
id author Umar Muhammad
option name Hash type spin default 16 min 1 max 1024
uciok
position startpos moves e2e4 e7e5
go movetime 1000
...
info depth 13 seldepth 28 score cp 8 nodes 1181840 nps 1804335 time 655 hashfull 89 pv g1f3 b8c6 b1c3 g8f6 f1d3 f8c5 e1g1 e8g8 c3d5 d7d6 c2c4 c8e6 f3g5
bestmove g1f3
```

Supported: `uci`, `isready`, `ucinewgame`, `setoption name Hash value N`, `position [startpos | fen …] [moves …]`, `go [depth | movetime | wtime/btime/winc/binc/movestogo | nodes | infinite | perft N]`, `stop`, `quit`, plus `d` (print FEN) and `perft N` for debugging.

To measure strength changes, run engine-vs-engine matches with [cutechess-cli](https://github.com/cutechess/cutechess) and SPRT rather than judging by eye.

---

## Roadmap

- [x] UCI (Universal Chess Interface) protocol support
- [x] Quiescence Search
- [x] Piece-Square Tables (PeSTO tapered evaluation)
- [x] Magic Bitboards
- [x] Null Move Pruning, LMR, killer/history ordering
- [x] Iterative Deepening with time management
- [x] Dear ImGui overlay (evaluation, search depth, PV line)
- [ ] Pawn structure, king safety and mobility terms in the evaluation
- [ ] Lazy SMP multi-threaded search
- [ ] Opening book / Syzygy tablebases
- [ ] SDL3 port (once distro packages are widely available)
- [ ] Android build via CMake NDK toolchain

---

## License

MIT License — feel free to fork, learn from, and build upon this project.
