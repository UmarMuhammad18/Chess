# Chess Engine

A cross-platform chess engine and GUI built from scratch in **C++20** using **SDL2**, **OpenGL 3.3 Core** (GLES 3.0 on mobile), and a fully custom AI search stack.

Built as a learning and CV project — every layer was implemented by hand with no chess library dependencies.

---

## Architecture

The project is cleanly split into three isolated layers:

```
src/
├── engine/       ← Pure C++ (no OS, no SDL, no OpenGL)
├── platform/     ← SDL2 windowing, input, timing
├── render/       ← OpenGL/GLES shader pipeline, textures, mesh
└── game/         ← Integration: connects engine ↔ platform ↔ renderer
```

### Engine Layer (Pure C++)
| File | Responsibility |
|---|---|
| `types.hpp` | `U64`, `Square`, `Piece`, `Color`, 16-bit `Move` with `MoveFlag` |
| `utils.hpp` | `popcount`, `lsb`, `pop_lsb` via C++20 `<bit>` |
| `bitboard.hpp` | Inline `set_bit`, `clear_bit`, `get_bit` |
| `board.hpp/cpp` | Board state, FEN parser, history stack, `make_move`, `unmake_move` |
| `movegen.hpp/cpp` | Precomputed leaper attacks, classical ray attacks, `is_square_attacked`, legal move generation |
| `eval.hpp/cpp` | Material-only evaluation (centipawn scale) |
| `search.hpp/cpp` | Negamax + Alpha-Beta pruning + Move Ordering (MVV-LVA + TT Best Move) |
| `zobrist.hpp/cpp` | 64-bit Zobrist hashing, incremental XOR updates |
| `transposition.hpp/cpp` | 16MB hash table with Exact/Alpha/Beta bounds |

### Platform Layer (SDL2)
| File | Responsibility |
|---|---|
| `sdl_context.hpp/cpp` | SDL2 window, OpenGL context creation (3.3 Core, MSAA 4x, VSync) |
| `app.hpp/cpp` | Main event loop, SDL_PollEvent |
| `input.hpp/cpp` | Mouse state, `is_mouse_just_pressed`, screen coordinates |

### Render Layer (OpenGL 3.3 / GLES 3.0)
| File | Responsibility |
|---|---|
| `gl_context.hpp/cpp` | GLEW initialization, blending, clear |
| `shader.hpp/cpp` | Vertex/fragment shader compilation, linking, error checking |
| `texture.hpp/cpp` | PNG loading via `stb_image`, `GL_TEXTURE_2D` creation |
| `mesh.hpp/cpp` | VAO/VBO/EBO for a textured 2D quad |
| `renderer.hpp/cpp` | Board draw, piece sprite atlas UV mapping, selection highlight |

---

## Features

- **Bitboard Representation** — 64-bit integer board state for all 12 piece types
- **FEN Parser** — Load any board position using standard Forsyth-Edwards Notation
- **Full Legal Move Generation** — Pawns (pushes, captures, en passant, promotions ×4), Knights, Bishops, Rooks, Queens, Kings, Castling (K+Q-side)
- **King Safety Filtering** — `is_square_attacked()` using the Super Piece technique
- **State History Stack** — Reversible `make_move` / `unmake_move` with full state restoration
- **Zobrist Hashing** — Incremental XOR hashing for near-instant position identification
- **Transposition Table** — 16MB hash table (Exact/Alpha/Beta bounds) to avoid re-searching known positions
- **Alpha-Beta Negamax Search** — Recursive Minimax with pruning at depth 4
- **Move Ordering** — MVV-LVA capture ordering + TT Best Move promotion
- **OpenGL Renderer** — Sprite atlas UV mapping, per-square viewport positioning
- **Interactive GUI** — Click to select and move pieces; AI responds automatically as Black

---

## Tech Stack

| Layer | Technology |
|---|---|
| Language | C++20 |
| Build | CMake 3.16+ |
| Windowing | SDL2 |
| Rendering | OpenGL 3.3 Core (Desktop) / GLES 3.0 (Mobile) |
| Extension Loader | GLEW |
| Image Loading | stb_image (single-header) |
| Testing | GoogleTest (FetchContent) |

---

## Build Instructions

### Prerequisites

**Windows** (recommended via [vcpkg](https://github.com/microsoft/vcpkg)):
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

### Configure & Build

```bash
# Configure (Release mode)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release

# Run the GUI
./build/chess_engine       # Linux/macOS
.\build\Release\chess_engine.exe  # Windows
```

### Run Tests

```bash
ctest --test-dir build --output-on-failure
```

Tests include:
- ✅ FEN parsing correctness
- ✅ 20 legal moves from the starting position (Perft 1)
- ✅ Zobrist hash reversibility after `make_move` + `unmake_move`

---

## Project Structure

```
Chess/
├── CMakeLists.txt
├── README.md
├── .gitignore
├── assets/
│   ├── shaders/
│   │   ├── basic.vert
│   │   └── basic.frag
│   └── textures/
│       ├── chess_board.png
│       └── pieces.png
├── src/
│   ├── main.cpp
│   ├── vendor/
│   │   └── stb_image.h
│   ├── engine/
│   ├── platform/
│   ├── render/
│   └── game/
└── test/
    └── engine_test.cpp
```

---

## Roadmap

- [ ] UCI (Universal Chess Interface) protocol support
- [ ] Quiescence Search (captures-only terminal nodes)
- [ ] Piece-Square Tables (positional evaluation)
- [ ] Magic Bitboards (O(1) slider attack generation)
- [ ] Null Move Pruning
- [ ] Iterative Deepening with time management
- [ ] Android build via CMake NDK toolchain
- [ ] Dear ImGui debug overlay (evaluation, search depth, PV line)

---

## License

MIT License — feel free to fork, learn from, and build upon this project.
