cmake_minimum_required(VERSION 3.15)

add_subdirectory(external)

project(K_means CXX)

add_executable(${PROJECT_NAME} example.cpp)

target_compile_features (${PROJECT_NAME} PRIVATE cxx_std_20)

target_link_libraries(${PROJECT_NAME} range-v3)
