cmake_minimum_required(VERSION 3.22)
project(compress-ocl)

set(CMAKE_CXX_STANDARD 17)

add_executable(compress-ocl
    src/solution.cpp)
find_package(OpenCLHeaders REQUIRED)
find_package(OpenCLICDLoader REQUIRED)
find_package(OpenCLHeadersCpp REQUIRED)
find_package(LibLZMA REQUIRED)
target_include_directories(compress-ocl PRIVATE src)
target_link_libraries(compress-ocl PRIVATE LibLZMA::LibLZMA OpenCL::Headers OpenCL::OpenCL OpenCL::HeadersCpp)
