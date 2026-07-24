cmake_minimum_required(VERSION 3.16)
project(ChessEngine CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

include_directories(${SDL2_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS} src)

file(GLOB_RECURSE SRC_FILES 
    "src/engine/*.cpp"
    "src/platform/*.cpp"
    "src/render/*.cpp"
    "src/game/*.cpp"
)

add_executable(chess_engine src/main.cpp ${SRC_FILES})
target_link_libraries(chess_engine ${SDL2_LIBRARIES} ${OPENGL_LIBRARIES} GLEW::GLEW)

# Copy assets to build dir
add_custom_command(TARGET chess_engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:chess_engine /assets
)

# GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_executable(engine_test test/engine_test.cpp src/engine/board.cpp src/engine/bitboard.cpp src/engine/movegen.cpp src/engine/search.cpp src/engine/eval.cpp src/engine/zobrist.cpp src/engine/transposition.cpp)
target_link_libraries(engine_test GTest::gtest_main)
add_test(NAME EngineTests COMMAND engine_test)
