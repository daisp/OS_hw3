cmake_minimum_required(VERSION 3.10)
project(hw3)

set(CMAKE_CXX_STANDARD 11)
set(GCC_COVERAGE_COMPILE_FLAGS "-pthread -g")

add_executable(hw3 Factory.cxx Factory.h Product.h test.cxx test_utilities.h)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}" )