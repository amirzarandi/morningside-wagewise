cmake_minimum_required(VERSION 3.10.0)
project(morningside-wagerwise VERSION 0.1.0 LANGUAGES C CXX)

add_executable(morningside-wagerwise OrderBook.cpp)

include(CTest)
enable_testing()

