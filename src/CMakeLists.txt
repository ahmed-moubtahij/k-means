add_executable(demo demo.cpp)

target_link_libraries(
  demo
  PRIVATE
    kmn
    project_options
    project_warnings
    fmt::fmt
    range-v3::range-v3)

if(${PROJECT_NAME}_ENABLE_CLANG_TIDY)
    set_target_properties(demo PROPERTIES CXX_CLANG_TIDY "${${PROJECT_NAME}_CLANG_TIDY_COMMAND}")
    message(STATUS "set target properties for clang tidy")
endif()
