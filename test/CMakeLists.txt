project(ubench-test CXX)

file(GLOB SOURCES *.cxx)

add_executable(ubench-test ${SOURCES})
target_compile_features(ubench PUBLIC cxx_std_17)
target_compile_options(ubench PRIVATE -Wall -Wextra)

target_link_libraries(ubench-test PUBLIC ubench)
