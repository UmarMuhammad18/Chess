cmake_minimum_required(VERSION 3.16)
project(ChessEngine CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(CHESS_ANDROID "Build Android / GLES stubs (no desktop SDL GUI)" OFF)

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

find_package(Threads REQUIRED)

if(CHESS_ANDROID)
    message(STATUS "CHESS_ANDROID=ON — building engine + UCI only (GLES stubs in tree)")
    add_compile_definitions(CHESS_ANDROID=1)
    add_executable(chess_uci src/uci/uci.cpp ${ENGINE_SRC})
    target_include_directories(chess_uci PRIVATE src)
    target_link_libraries(chess_uci Threads::Threads)
else()
    find_package(SDL2 REQUIRED)
    find_package(OpenGL REQUIRED)
    find_package(GLEW REQUIRED)

    include_directories(${SDL2_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS} src)

    file(GLOB PLATFORM_SRC src/platform/*.cpp)
    file(GLOB RENDER_SRC src/render/*.cpp)
    file(GLOB GAME_SRC src/game/*.cpp)

    add_executable(chess_engine src/main.cpp ${ENGINE_SRC} ${PLATFORM_SRC} ${RENDER_SRC} ${GAME_SRC})
    target_link_libraries(chess_engine ${SDL2_LIBRARIES} ${OPENGL_LIBRARIES} GLEW::GLEW Threads::Threads)

    add_executable(chess_uci src/uci/uci.cpp ${ENGINE_SRC})
    target_link_libraries(chess_uci Threads::Threads)

    add_custom_command(TARGET chess_engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:chess_engine>/assets
    )
endif()

if(NOT CHESS_ANDROID)
    include(FetchContent)
    FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    add_executable(engine_test test/engine_test.cpp ${ENGINE_SRC})
    target_include_directories(engine_test PRIVATE src)
    target_link_libraries(engine_test GTest::gtest_main Threads::Threads)
    add_test(NAME EngineTests COMMAND engine_test)
endif()
