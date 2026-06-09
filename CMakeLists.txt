cmake_minimum_required(VERSION 3.16)
project(OrderBook LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(orderbook
    src/main.cpp
    src/orderbook.cpp
    src/cli.cpp
)

target_include_directories(orderbook PRIVATE include)
target_compile_options(orderbook PRIVATE -Wall -Wextra)
