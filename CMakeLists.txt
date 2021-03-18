cmake_minimum_required(VERSION 3.15)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
    CACHE PATH "Path to vcpkg toolchain")

project(k-means CXX)
include(cmake/StandardProjectSettings.cmake)

add_library(kmn INTERFACE)
target_include_directories(kmn INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Link this 'library' to set the c++ standard / compile-time options requested
add_library(project_options INTERFACE)
target_compile_features(project_options INTERFACE cxx_std_20)

# Link this 'library' to use the warnings specified in CompilerWarnings.cmake
add_library(project_warnings INTERFACE)

# enable cache system
include(cmake/Cache.cmake)

# standard compiler warnings
include(cmake/CompilerWarnings.cmake)
set_project_warnings(project_warnings)

# sanitizer options if supported by compiler
include(cmake/Sanitizers.cmake)
enable_sanitizers(project_options)

# allow for static analysis options
include(cmake/StaticAnalyzers.cmake)

# find vcpkg packages
find_package(fmt CONFIG REQUIRED)
find_package(range-v3 CONFIG REQUIRED)

add_subdirectory(src)
