cmake_minimum_required(VERSION 3.10)
project(ubench CXX)

file(GLOB_RECURSE HEADERS include/*.h lib/*.h)
file(GLOB_RECURSE SOURCES lib/*.cxx)

add_library(ubench STATIC ${HEADERS} ${SOURCES})
target_compile_features(ubench PUBLIC cxx_std_17)
target_compile_options(ubench PRIVATE -Wall -Wextra)
target_include_directories(ubench PUBLIC include)

add_subdirectory(test)
