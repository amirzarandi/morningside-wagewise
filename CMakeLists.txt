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
    src/marketdata/MarketDataFeedHandler.cpp
)

target_include_directories(morningside-wagerwise PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    $<TARGET_PROPERTY:common,INTERFACE_INCLUDE_DIRECTORIES>
)

include(FetchContent)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)

find_package(CURL REQUIRED)

target_link_libraries(morningside-wagerwise 
    PRIVATE nlohmann_json::nlohmann_json
    PRIVATE common
    PRIVATE CURL::libcurl
)

include(CTest)
enable_testing()