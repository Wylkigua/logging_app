cmake_minimum_required(VERSION 3.18)
project(Tests_logger_lib LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(tests_logger_lib tests.cpp)
add_subdirectory(../ logger_lib_build)
target_link_libraries(tests_logger_lib PRIVATE logger_shared)

enable_testing()
add_test(NAME Test_logger_lib COMMAND tests_logger_lib)
