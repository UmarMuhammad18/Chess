cmake_minimum_required(VERSION 3.20)
project(ChessEngine VERSION 0.2.0 LANGUAGES CXX)

option(CHESS_BUILD_GUI          "Build the SDL2/OpenGL desktop GUI"             ON)
option(CHESS_BUILD_TESTS        "Build the GoogleTest suite"                     ON)
option(CHESS_ENABLE_SANITIZERS  "Build with AddressSanitizer + UBSan (GCC/Clang)" OFF)
option(CHESS_WARNINGS_AS_ERRORS "Treat compiler warnings as errors"              OFF)
option(CHESS_ANDROID            "Engine + UCI only, for Android NDK toolchains"  OFF)
option(CHESS_IOS                "Engine + C API static library for Xcode"        OFF)

# Mobile builds have no SDL desktop GUI; see android/README.md and ios/README.md
if(CHESS_ANDROID OR CHESS_IOS)
    set(CHESS_BUILD_GUI OFF CACHE BOOL "" FORCE)
    set(CHESS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

include(FetchContent)
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

find_package(Threads REQUIRED)

# ── Shared compiler settings for our own targets (not third-party code) ──
add_library(chess_options INTERFACE)
if(MSVC)
    target_compile_options(chess_options INTERFACE /W4 /permissive- /utf-8)
    if(CHESS_WARNINGS_AS_ERRORS)
        target_compile_options(chess_options INTERFACE /WX)
    endif()
else()
    target_compile_options(chess_options INTERFACE -Wall -Wextra -Wpedantic)
    if(CHESS_WARNINGS_AS_ERRORS)
        target_compile_options(chess_options INTERFACE -Werror)
    endif()
    if(CHESS_ENABLE_SANITIZERS)
        target_compile_options(chess_options INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
        target_link_options(chess_options INTERFACE -fsanitize=address,undefined)
    endif()
endif()

# ── Engine core: pure C++, no OS / SDL / OpenGL ──
add_library(chess_core STATIC
    src/engine/attacks.cpp
    src/engine/board.cpp
    src/engine/eval.cpp
    src/engine/movegen.cpp
    src/engine/notation.cpp
    src/engine/search.cpp
    src/engine/transposition.cpp
    src/engine/zobrist.cpp
    src/uci/uci.cpp
    src/bridge/chess_c_api.cpp
)
target_include_directories(chess_core PUBLIC src src/bridge)
target_compile_features(chess_core PUBLIC cxx_std_20)
target_link_libraries(chess_core PUBLIC chess_options Threads::Threads)
set_target_properties(chess_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ── UCI engine binary (for Arena, Cute Chess, lichess-bot, ...) ──
add_executable(chess_uci src/uci_main.cpp)
target_link_libraries(chess_uci PRIVATE chess_core)

# ── Desktop GUI ──
if(CHESS_BUILD_GUI)
    find_package(SDL2 CONFIG REQUIRED)
    find_package(OpenGL REQUIRED)
    find_package(GLEW REQUIRED)

    if(TARGET SDL2::SDL2)
        set(CHESS_SDL2_TARGET SDL2::SDL2)
    else()
        set(CHESS_SDL2_TARGET SDL2::SDL2-static)
    endif()

    FetchContent_Declare(
        imgui
        URL https://github.com/ocornut/imgui/archive/refs/tags/v1.91.5.zip
    )
    FetchContent_MakeAvailable(imgui)

    add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    )
    target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
    target_link_libraries(imgui PUBLIC ${CHESS_SDL2_TARGET} OpenGL::GL)

    add_executable(chess_engine
        src/main.cpp
        src/game/game.cpp
        src/platform/app.cpp
        src/platform/input.cpp
        src/platform/sdl_context.cpp
        src/render/gl_context.cpp
        src/render/mesh.cpp
        src/render/renderer.cpp
        src/render/shader.cpp
        src/render/texture.cpp
    )
    target_link_libraries(chess_engine PRIVATE chess_core imgui ${CHESS_SDL2_TARGET} OpenGL::GL GLEW::GLEW)

    # Copy assets next to the executable (the GUI resolves them relative to its own path)
    add_custom_command(TARGET chess_engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:chess_engine>/assets
    )
endif()

# ── Tests ──
if(CHESS_BUILD_TESTS)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    add_executable(engine_test test/engine_test.cpp test/search_test.cpp test/uci_test.cpp test/bridge_test.cpp)
    target_link_libraries(engine_test PRIVATE chess_core GTest::gtest_main)

    # Fast suite runs on every build; deep perft is labelled "slow" (ctest -LE slow to skip)
    add_test(NAME engine_tests COMMAND engine_test --gtest_filter=-*Slow*)
    add_test(NAME perft_deep   COMMAND engine_test --gtest_filter=*Slow*)
    set_tests_properties(perft_deep PROPERTIES LABELS slow)
endif()
