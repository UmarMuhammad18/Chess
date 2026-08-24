cmake_minimum_required(VERSION 3.16)
project(ChessEngine CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(Threads REQUIRED)

include_directories(${SDL2_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS} src)

set(ENGINE_SRC
    src/engine/bitboard.cpp
    src/engine/board.cpp
    src/engine/eval.cpp
    src/engine/movegen.cpp
    src/engine/notation.cpp
    src/engine/search.cpp
    src/engine/transposition.cpp
    src/engine/zobrist.cpp
)

file(GLOB PLATFORM_SRC src/platform/*.cpp)
file(GLOB RENDER_SRC src/render/*.cpp)
file(GLOB GAME_SRC src/game/*.cpp)

add_executable(chess_engine src/main.cpp ${ENGINE_SRC} ${PLATFORM_SRC} ${RENDER_SRC} ${GAME_SRC})
target_link_libraries(chess_engine ${SDL2_LIBRARIES} ${OPENGL_LIBRARIES} GLEW::GLEW Threads::Threads)

add_executable(chess_uci src/uci/uci.cpp ${ENGINE_SRC})
target_link_libraries(chess_uci Threads::Threads)

# Copy assets to build dir
add_custom_command(TARGET chess_engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:chess_engine>/assets
)

# GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_executable(engine_test test/engine_test.cpp ${ENGINE_SRC})
target_link_libraries(engine_test GTest::gtest_main Threads::Threads)
add_test(NAME EngineTests COMMAND engine_test)
