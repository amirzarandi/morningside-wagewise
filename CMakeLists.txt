cmake_minimum_required(VERSION 3.10.0)
project(morningside VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-O2)

add_library(common INTERFACE)
target_include_directories(common INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/common
)

add_executable(morningside-wagewise
    main.cpp
    src/orderbook/Orderbook.cpp
    src/marketdata/MarketDataFeedHandler.cpp
)

target_include_directories(morningside-wagewise PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    $<TARGET_PROPERTY:common,INTERFACE_INCLUDE_DIRECTORIES>
)

include(FetchContent)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)

find_package(CURL REQUIRED)

target_link_libraries(morningside-wagewise
    PRIVATE nlohmann_json::nlohmann_json
    PRIVATE common
    PRIVATE CURL::libcurl
)

set(BENCHMARK_ENABLE_TESTING OFF)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF)

FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.9.4
)
FetchContent_MakeAvailable(benchmark)

file(GLOB PERF_SOURCES perf/*.cpp)
foreach(sourcefile ${PERF_SOURCES})
    get_filename_component(name ${sourcefile} NAME_WE)
    add_executable(${name} 
        ${sourcefile}
        src/orderbook/Orderbook.cpp
        src/marketdata/MarketDataFeedHandler.cpp
    )
    target_include_directories(${name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        $<TARGET_PROPERTY:common,INTERFACE_INCLUDE_DIRECTORIES>
    )
    
    # Link only benchmark, not benchmark_main (BENCHMARK_MAIN is in the source)
    target_link_libraries(${name}
        PRIVATE common
        PRIVATE benchmark::benchmark
        PRIVATE CURL::libcurl
        PRIVATE nlohmann_json::nlohmann_json
    )
    
    # Ensure Release build for benchmarks
    set_target_properties(${name} PROPERTIES
        CMAKE_CXX_FLAGS_DEBUG "-O2 -g"
    )
endforeach(sourcefile ${PERF_SOURCES})

include(CTest)
enable_testing()