cmake_minimum_required(VERSION 3.15)

set(
	CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
	CACHE PATH "Path to vcpkg toolchain")

project(k-means CXX)

add_executable(${PROJECT_NAME} example.cpp "DataPoint.hpp")

find_package(fmt CONFIG REQUIRED)
find_package(range-v3 CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    fmt::fmt
    range-v3::range-v3)

target_compile_features (${PROJECT_NAME} PRIVATE cxx_std_20)
