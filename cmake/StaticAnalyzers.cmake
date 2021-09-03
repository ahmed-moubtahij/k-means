option(ENABLE_CPPCHECK "Enable static analysis with cppcheck" ON)
option(${PROJECT_NAME}_ENABLE_CLANG_TIDY "Builds with clang-tidy, if available. Defaults to ON." OFF)
option(ENABLE_INCLUDE_WHAT_YOU_USE "Enable static analysis with include-what-you-use" OFF)

if(ENABLE_CPPCHECK)
  find_program(CPPCHECK_PATH cppcheck /usr/local/share/Cppcheck)
  if(CPPCHECK_PATH)
    set(CMAKE_CXX_CPPCHECK
        ${CPPCHECK_PATH}
        --enable=all
        --suppress=missingInclude
        --inline-suppr
        --inconclusive
        -i
        ${CMAKE_SOURCE_DIR}/imgui/lib)
    message(STATUS "Cppcheck from ${CPPCHECK_PATH} is set")
  else()
    message(SEND_ERROR "cppcheck requested but executable not found")
  endif()
endif()

if(${PROJECT_NAME}_ENABLE_CLANG_TIDY)
  find_program(CLANG_TIDY_PATH clang-tidy)
  if(CLANG_TIDY_PATH)
    set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_PATH} -extra-arg=-Wno-unknown-warning-option)
    set(CLANG_TIDY_COMMAND "${CLANG_TIDY_PATH};--system-headers")
    message(STATUS "Clang tidy from ${CLANG_TIDY_PATH} is set.")
  else()
    message(SEND_ERROR "clang-tidy requested but executable not found")
  endif()
endif()

if(ENABLE_INCLUDE_WHAT_YOU_USE)
  find_program(INCLUDE_WHAT_YOU_USE include-what-you-use)
  if(INCLUDE_WHAT_YOU_USE)
    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${INCLUDE_WHAT_YOU_USE})
  else()
    message(SEND_ERROR "include-what-you-use requested but executable not found")
  endif()
endif()

