cmake_minimum_required(VERSION 3.12)
project(logic)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# tests
add_subdirectory(extern/googletest)
include(GoogleTest)
include (CTest)
enable_testing()
add_subdirectory(tests)