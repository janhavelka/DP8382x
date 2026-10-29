cmake_minimum_required(VERSION 3.16)
project(DP8382x LANGUAGES CXX)

# Passive public values have no compiled implementation or SDK dependency.
add_library(DP8382x INTERFACE)
add_library(DP8382x::DP8382x ALIAS DP8382x)
target_compile_features(DP8382x INTERFACE cxx_std_17)
target_include_directories(DP8382x INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")
target_sources(DP8382x INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/DP8382x/Status.h>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/DP8382x/Config.h>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/DP8382x/CommandTable.h>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/DP8382x/Version.h>")

option(DP8382X_BUILD_TESTS "Build portable foundation regression tests" OFF)
if(DP8382X_BUILD_TESTS)
  enable_testing()
  add_executable(dp8382x_foundation_tests test/foundation.cpp)
  target_link_libraries(dp8382x_foundation_tests PRIVATE DP8382x)

  # Separate translation units catch accidental transitive include requirements.
  add_library(dp8382x_header_checks OBJECT
    test/header_status.cpp
    test/header_config.cpp
    test/header_command_table.cpp
    test/header_version.cpp)
  target_link_libraries(dp8382x_header_checks PRIVATE DP8382x)

  foreach(target IN ITEMS dp8382x_foundation_tests dp8382x_header_checks)
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)
    if(MSVC)
      target_compile_options(${target} PRIVATE /W4 /WX /permissive-)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
      target_compile_options(${target} PRIVATE
        -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion)
    endif()
  endforeach()

  foreach(suite IN ITEMS status link masks passive)
    add_test(NAME foundation_${suite} COMMAND dp8382x_foundation_tests ${suite})
    set_tests_properties(foundation_${suite} PROPERTIES TIMEOUT 30)
  endforeach()
endif()
