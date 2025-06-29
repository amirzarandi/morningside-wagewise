cmake_minimum_required(VERSION 3.10.0)

project(morningside-wagerwise VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(common INTERFACE)

target_include_directories(common INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/common
)

add_executable(morningside-wagerwise
    main.cpp
    src/orderbook/Orderbook.cpp
)

target_include_directories(morningside-wagerwise PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    $<TARGET_PROPERTY:common,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(morningside-wagerwise PRIVATE common)

include(CTest)
enable_testing()