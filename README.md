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
| `libchess_core.a` | Static engine + C API for iOS/Swift (`-DCHESS_IOS=ON`) |

---

## Play the GUI

| Control | Action |
|---|---|
| Click piece / square | Select and move; under-promotion picker Q/R/B/N |
| **New Game** / `N` | Reset |
| **Undo** / `U` | Take back |
| **Resign** / `R` | Resign |
| **White / Black** | Side to play |
| **0.5s–5s** | Think time |
| **Analysis** | Search without auto-play |
| **Export PGN** | Write `game.pgn` + stdout |

Eval bar + multi-move PV shown in the side panel.

---

## Engine features

- Bitboards, FEN, legal movegen with **magic bitboards**
- Make/unmake, null-move, draws, Zobrist + 32MB TT
- Search: ID, PVS, null-move, LMR, killers, history, aspiration, reverse futility + futility pruning, long PV
- Eval: material, PSTs, bishop pair, mobility, pawn structure, open files
- PGN helpers + UCI + **C API for mobile**

---

## iPhone / iOS scaffolding

You need a **Mac + Xcode** to install on an iPhone. The engine exposes a **C API** for SwiftUI.

```bash
cmake -B build-ios -DCHESS_IOS=ON
cmake --build build-ios   # libchess_core.a + chess_uci
```

See **`ios/README.md`** for the full plan (Swift bridge, device install, milestones).

---

## Android scaffolding

```bash
cmake -B build-android -DCHESS_ANDROID=ON
cmake --build build-android   # chess_uci only
```

See `android/README.md` for NDK / GLES steps.

---

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./build/chess_engine
ctest --test-dir build --output-on-failure
```

---

## Roadmap

- [x] Playable GUI, UCI, search/eval upgrades, magic bitboards
- [x] Analysis mode, under-promotion, PGN export
- [x] Android / GLES scaffolding (CMake + docs + GL stubs)
- [x] iOS C API + Swift wrapper scaffolding
- [ ] Full iPhone SwiftUI app (needs Mac/Xcode)
- [ ] Full Android NativeActivity / APK

---

## License

MIT License.
