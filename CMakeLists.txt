cmake_minimum_required(VERSION 3.16)
project(OrderBook LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Engine (shared between app and tests) ---
add_library(engine STATIC
    src/orderbook.cpp
    src/cli.cpp
)
target_include_directories(engine PUBLIC include)
target_compile_options(engine PRIVATE -Wall -Wextra)

# --- App ---
add_executable(orderbook src/main.cpp)
target_link_libraries(orderbook PRIVATE engine)
target_compile_options(orderbook PRIVATE -Wall -Wextra)

# --- Catch2 ---
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1
)
FetchContent_MakeAvailable(Catch2)

# --- Tests ---
enable_testing()

add_executable(tests tests/test_orderbook.cpp)
target_link_libraries(tests PRIVATE engine Catch2::Catch2WithMain)
target_compile_options(tests PRIVATE -Wall -Wextra)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(tests)
