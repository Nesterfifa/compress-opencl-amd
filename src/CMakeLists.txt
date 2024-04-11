add_executable(compress-ocl-src unify.cpp)

include_directories(${PROJECT_SOURCE_DIR}/src)

target_link_libraries(compress-ocl-src PRIVATE LibLZMA::LibLZMA OpenCL::OpenCL)