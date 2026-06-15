cmake_minimum_required(VERSION 3.10)
project(ParallelSCCVGC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenMP REQUIRED)

include_directories(src)

add_executable(parallel_scc 
    src/main.cpp
    src/graph/graph.cpp
    src/scc/tarjan.cpp
    src/scc/bgss.cpp
    src/scc/vgc.cpp
    src/scc/multi_bgss.cpp
    src/scc/hashbag.cpp
    src/benchmark/metrics.cpp
)

add_executable(gen_data
    src/generate_datasets.cpp
    src/graph/graph.cpp
)

target_link_libraries(parallel_scc OpenMP::OpenMP_CXX)
target_link_libraries(gen_data OpenMP::OpenMP_CXX)

add_executable(test_correctness
    tests/test_correctness.cpp
    src/graph/graph.cpp
    src/scc/tarjan.cpp
    src/scc/bgss.cpp
    src/scc/vgc.cpp
    src/scc/multi_bgss.cpp
)
target_link_libraries(test_correctness OpenMP::OpenMP_CXX)

enable_testing()
add_test(NAME CorrectnessTest COMMAND test_correctness)
