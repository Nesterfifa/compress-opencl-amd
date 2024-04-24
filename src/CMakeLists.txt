add_executable(solution solution.cpp)
add_executable(unify unify.cpp)

include_directories(${PROJECT_SOURCE_DIR}/src)

target_link_libraries(solution PRIVATE LibLZMA::LibLZMA OpenCL::OpenCL)
target_link_libraries(unify PRIVATE LibLZMA::LibLZMA OpenCL::OpenCL)
