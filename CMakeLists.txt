cmake_minimum_required(VERSION 3.15)

if(NOT DEFINED VCPKG_ROOT)
	if(DEFINED ENV{VCPKG_ROOT})
		set(VCPKG_ROOT $ENV{VCPKG_ROOT} CACHE PATH "Path to vcpkg directory")
	endif()
endif()

if(DEFINED VCPKG_ROOT)
	set(
		CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
		CACHE PATH "Path to vcpkg toolchain")

	if(NOT EXISTS "${CMAKE_TOOLCHAIN_FILE}")
		message(FATAL_ERROR "Invalid VCPKG_ROOT specified (${VCPKG_ROOT}); please specify an absolute path")
	endif()
endif()

project(K_means CXX)

add_subdirectory(external)

add_executable(${PROJECT_NAME} example.cpp "DataPoint.hpp")

vcpkg_install_packages()

find_package(fmt CONFIG REQUIRED)
find_package(range-v3 CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    fmt::fmt
    range-v3::range-v3)

target_compile_features (${PROJECT_NAME} PRIVATE cxx_std_20)