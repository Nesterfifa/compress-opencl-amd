cmake_minimum_required(VERSION 3.22)
project(compress-ocl)

set(CMAKE_CXX_STANDARD 20)

find_package(OpenCL REQUIRED)
find_package(LibLZMA REQUIRED)

enable_testing()
add_subdirectory(test)
add_subdirectory(src)
