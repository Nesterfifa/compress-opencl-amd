add_executable(compress-ocl-test ${PROJECT_SOURCE_DIR}/test/compress_test.cc)

include_directories(${PROJECT_SOURCE_DIR}/src)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

FetchContent_MakeAvailable(googletest)

include(GoogleTest)
target_link_libraries(compress-ocl-test PRIVATE GTest::gtest_main LibLZMA::LibLZMA OpenCL::OpenCL)
gtest_discover_tests(compress-ocl-test)