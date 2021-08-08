cmake_minimum_required(VERSION 3.10)
set(TARGET_NAME dll_monitor)
project(${TARGET_NAME} LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(./external/cpp-utils)
add_subdirectory(./external/fmt)
file(GLOB LIB_MINITOR_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

include_directories(./)
include_directories(./external/cpp-utils/include)
include_directories(./external/cxxopts/include)
include_directories(./external/fmt/include)

add_executable(${TARGET_NAME} ${LIB_MINITOR_SRCS})
target_link_libraries(${TARGET_NAME} cpp-utils fmt)

