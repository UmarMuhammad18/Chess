# iPhone / iOS scaffolding

You only have an iPhone — this is the path from the C++ engine to a playable app on your device.

## Reality check (important)

| Requirement | Why |
|-------------|-----|
| **A Mac with Xcode** | Apple only allows iOS builds from macOS (or paid cloud Mac CI) |
| **Apple ID** | Free account: install on *your* iPhone for ~7 days per build |
| **Paid Developer Program** (~£79/year) | Optional; needed for long-term install & App Store |

You **cannot** compile an iPhone app on Windows/Linux alone. The engine code here is ready; packaging needs Xcode on a Mac (university lab Mac, friend’s Mac, or MacinCloud-style CI).

## Architecture (recommended)

```
SwiftUI (board UI, touch)
    ↓
C API  (src/bridge/chess_c_api.h)   ← stable boundary
    ↓
C++ engine (src/engine/)            ← already done
```

Do **not** rewrite the engine in Swift. Keep chess logic in C++; use a thin C bridge so Swift can call it.

## What’s in this repo

| Path | Purpose |
|------|---------|
| `src/bridge/chess_c_api.h` | C API for Swift |
| `src/bridge/chess_c_api.cpp` | Implementation |
| `ios/ChessBridge/` | Example Swift wrapper + bridging header |
| `-DCHESS_IOS=ON` | CMake builds `libchess_core.a` + `chess_uci` |

## Build the static library (on a Mac)

```bash
cmake -B build-ios -DCHESS_IOS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-ios

# Outputs:
#   build-ios/libchess_core.a
#   build-ios/chess_uci
```

For a **real iPhone** device archive you typically use an Xcode project that:

1. Adds all `src/engine/*.cpp` + `src/bridge/chess_c_api.cpp` as compile sources, **or**
2. Links prebuilt `libchess_core.a` for `iphoneos` / `iphonesimulator` (XCFramework).

## Minimal Swift usage

```swift
let eng = chess_create()
defer { chess_destroy(eng) }

var move = [CChar](repeating: 0, count: 16)
let score = chess_search(eng, 1000, &move, 16)
let uci = String(cString: move)
print("best \(uci) score \(score)")
chess_make_uci(eng, uci)
```

See `ios/ChessBridge/ChessEngine.swift` for a small wrapper class.

## UI approach on iPhone

1. **SwiftUI board** — 8×8 grid; highlight legal moves from `chess_legal_moves`.
2. **Human move** — square taps → UCI → `chess_make_uci`.
3. **Engine reply** — background queue `chess_search`, apply on main thread.
4. **Assets** — SF Symbols or PNG pieces.

## Suggested milestone order

1. C API — **done in this PR**
2. Xcode app, simulator, board + legal moves
3. Touch to move + engine reply
4. Device install via free Apple ID
5. Polish (clock, PGN, analysis)

## CV bullet (honest)

> Cross-platform C++20 chess engine with UCI; desktop SDL/OpenGL client; C API and scaffolding for iOS (SwiftUI) and Android NDK.
