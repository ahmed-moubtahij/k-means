cmake_minimum_required(VERSION 3.15)

project(K_means CXX)

target_compile_features (${PROJECT_NAME} PRIVATE cxx_std_20)

add_library(range-v3 INTERFACE)

target_include_directories(external/range-v3/ INTERFACE include)

add_executable(${PROJECT_NAME} example.cpp)
