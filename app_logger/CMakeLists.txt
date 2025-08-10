cmake_minimum_required(VERSION 3.18)
project(Logger_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(logger_app ${SRC_FILES})

# Линкуем с библиотекой
add_subdirectory(../lib_logger logger_lib_build)
target_link_libraries(logger_app PRIVATE logger_shared)
