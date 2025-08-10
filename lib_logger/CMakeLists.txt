cmake_minimum_required(VERSION 3.18)
project(Logging_lib LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# статическая библиотека
add_library(logger_static STATIC ${SRC_FILES})

# динамическая библиотека
add_library(logger_shared SHARED ${SRC_FILES})

# заголовки
target_include_directories(logger_static PUBLIC
                           ${CMAKE_CURRENT_SOURCE_DIR}/src/include)
target_include_directories(logger_shared PUBLIC
                           ${CMAKE_CURRENT_SOURCE_DIR}/src/include)
