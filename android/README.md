# Android / GLES scaffolding

This directory documents the path from the desktop OpenGL 3.3 + SDL2 client to an Android NDK build.

## Status

| Piece | Status |
|-------|--------|
| Pure engine (`src/engine/`) | Ready — no OS deps |
| UCI binary | Builds with `-DCHESS_ANDROID=ON` |
| GLES 3.0 stubs | `src/render/gl_context.*` compiles with `CHESS_ANDROID` |
| NativeActivity / JNI | Not wired yet — see below |
| Touch input | Not wired yet |

## Build engine-only for Android toolchains

```bash
cmake -B build-android \
  -DCHESS_ANDROID=ON \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-24
cmake --build build-android
```

This produces `chess_uci`, which you can drive from a Java/Kotlin shell or pipe.

## Next implementation steps

1. **NativeActivity** or **GameActivity** entry in `android/app/`
2. Create EGL context + GLES 3.0 surface; map `GLContext` to GLES headers (`GLES3/gl3.h`)
3. Port shaders from GLSL 330 core → GLSL ES 300
4. Load assets from the APK asset manager
5. Map touch events → board clicks (reuse `pixel_to_square`)
6. Optional: expose the engine via JNI (`nativeSearch`, `nativeMakeMove`)

## Design notes

- Keep all chess logic in `src/engine/` — never pull Android APIs into the engine.
- The desktop GUI stays SDL2 + OpenGL 3.3; Android is a second front-end.
- Prefer one shared renderer abstraction (`Renderer`) with compile-time GLES vs GL backends.
