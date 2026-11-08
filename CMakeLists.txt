cmake_minimum_required(VERSION 3.16)
project(DP8382x LANGUAGES CXX)

# Portable implementation; no SDK, generated build step or optional tooling.
add_library(DP8382x STATIC src/DP8382x.cpp)
add_library(DP8382x::DP8382x ALIAS DP8382x)
target_compile_features(DP8382x PUBLIC cxx_std_17)
set_target_properties(DP8382x PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(DP8382x PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")
target_sources(DP8382x PRIVATE
  include/DP8382x/DP8382x.h
  include/DP8382x/Status.h
  include/DP8382x/Config.h
  include/DP8382x/CommandTable.h
  include/DP8382x/Version.h)

option(DP8382X_BUILD_TESTS "Build portable driver and foundation tests" OFF)
if(DP8382X_BUILD_TESTS)
  enable_testing()
  add_executable(dp8382x_foundation_tests test/foundation.cpp)
  target_link_libraries(dp8382x_foundation_tests PRIVATE DP8382x)
  add_executable(dp8382x_transport_tests test/transport.cpp)
  target_link_libraries(dp8382x_transport_tests PRIVATE DP8382x)
  add_executable(dp8382x_mmd_tests test/mmd.cpp)
  target_link_libraries(dp8382x_mmd_tests PRIVATE DP8382x)
  add_executable(dp8382x_access_tests test/access.cpp)
  target_link_libraries(dp8382x_access_tests PRIVATE DP8382x)
  add_executable(dp8382x_identity_tests test/identity.cpp)
  target_link_libraries(dp8382x_identity_tests PRIVATE DP8382x)

  add_executable(dp8382x_operation_tests test/operations.cpp)
  target_link_libraries(dp8382x_operation_tests PRIVATE DP8382x)

  add_executable(dp8382x_reset_tests test/reset.cpp)
  target_link_libraries(dp8382x_reset_tests PRIVATE DP8382x)

  # Separate translation units catch accidental transitive include requirements.
  add_library(dp8382x_header_checks OBJECT
    test/header_status.cpp
    test/header_config.cpp
    test/header_driver.cpp
    test/header_command_table.cpp
    test/header_version.cpp)
  target_link_libraries(dp8382x_header_checks PRIVATE DP8382x)

  foreach(target IN ITEMS DP8382x dp8382x_foundation_tests dp8382x_transport_tests dp8382x_mmd_tests dp8382x_access_tests dp8382x_identity_tests dp8382x_operation_tests dp8382x_reset_tests dp8382x_header_checks)
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)
    if(MSVC)
      target_compile_options(${target} PRIVATE /W4 /WX /permissive-)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
      target_compile_options(${target} PRIVATE
        -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion)
    endif()
  endforeach()

  foreach(target IN ITEMS dp8382x_foundation_tests dp8382x_transport_tests dp8382x_mmd_tests dp8382x_access_tests dp8382x_identity_tests dp8382x_operation_tests dp8382x_reset_tests)
    add_test(NAME ${target}_failure_path COMMAND ${target} --verify-failure-path)
    set_tests_properties(${target}_failure_path PROPERTIES WILL_FAIL TRUE TIMEOUT 30)
  endforeach()

  foreach(suite IN ITEMS status link masks passive)
    add_test(NAME foundation_${suite} COMMAND dp8382x_foundation_tests ${suite})
    set_tests_properties(foundation_${suite} PROPERTIES TIMEOUT 30)
  endforeach()
  foreach(suite IN ITEMS binding direct reports timing ownership)
    add_test(NAME transport_${suite} COMMAND dp8382x_transport_tests ${suite})
    set_tests_properties(transport_${suite} PROPERTIES TIMEOUT 30)
  endforeach()
  foreach(suite IN ITEMS traces admission failures reports ownership)
    add_test(NAME mmd_${suite} COMMAND dp8382x_mmd_tests ${suite})
    set_tests_properties(mmd_${suite} PROPERTIES TIMEOUT 30)
  endforeach()
  foreach(suite IN ITEMS compositions acquisition failures reports interrupts wake admission counters ownership)
    add_test(NAME access_${suite} COMMAND dp8382x_access_tests ${suite})
    set_tests_properties(access_${suite} PROPERTIES TIMEOUT 30)
  endforeach()
  foreach(suite IN ITEMS decode policy failures lifecycle ownership)
    add_test(NAME identity_${suite} COMMAND dp8382x_identity_tests ${suite})
    set_tests_properties(identity_${suite} PROPERTIES TIMEOUT 30)
  endforeach()
  foreach(suite IN ITEMS initialization admission timing failures lifecycle ownership)
    add_test(NAME operation_${suite} COMMAND dp8382x_operation_tests ${suite})
    set_tests_properties(operation_${suite} PROPERTIES TIMEOUT 30)
  endforeach()
  foreach(suite IN ITEMS traces floor failures timing external ownership)
    add_test(NAME reset_${suite} COMMAND dp8382x_reset_tests ${suite})
    set_tests_properties(reset_${suite} PROPERTIES TIMEOUT 30)
  endforeach()
endif()
