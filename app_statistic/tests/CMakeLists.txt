cmake_minimum_required(VERSION 3.16)
project(Test_statistic_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_executable(test_statistic_app tests.cpp ../src/statistic_app.cpp)

# Линкуем с библиотекой
add_subdirectory(../../lib_logger/ logger_lib_build)
target_link_libraries(test_statistic_app PRIVATE logger_shared)

enable_testing()
add_test(NAME Test_statistic_app COMMAND test_statistic_app)
