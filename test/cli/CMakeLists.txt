add_library(dp8382x_example_cli STATIC ../../examples/common/Dp8382xCli.cpp)
target_include_directories(dp8382x_example_cli PUBLIC ../../examples/common)
target_link_libraries(dp8382x_example_cli PUBLIC DP8382x)

add_executable(dp8382x_cli_fixture dp8382x_cli_fixture.cpp)
target_link_libraries(dp8382x_cli_fixture PRIVATE dp8382x_example_cli)
add_executable(dp8382x_console_tests console.cpp)
target_link_libraries(dp8382x_console_tests PRIVATE dp8382x_example_cli)
add_executable(dp8382x_cli_tests processor.cpp)
target_link_libraries(dp8382x_cli_tests PRIVATE dp8382x_example_cli)
foreach(target IN ITEMS dp8382x_example_cli dp8382x_cli_fixture dp8382x_console_tests dp8382x_cli_tests)
  set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /WX /UNDEBUG)
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
    target_compile_options(${target} PRIVATE
      -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion -UNDEBUG)
  endif()
endforeach()
foreach(suite IN ITEMS grammar loss cached retention routes)
  add_test(NAME cli_processor_${suite} COMMAND dp8382x_cli_tests ${suite})
  set_tests_properties(cli_processor_${suite} PROPERTIES TIMEOUT 15)
endforeach()

find_package(Python3 3.10 QUIET COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
  add_test(NAME cli_fixture_transcript COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/fixture_transcript.py" "$<TARGET_FILE:dp8382x_cli_fixture>")
  add_test(NAME cli_diagnostic_transcript COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/diagnostic_transcript.py" "$<TARGET_FILE:dp8382x_cli_fixture>")
  add_test(NAME cli_control_transcript COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/control_transcript.py" "$<TARGET_FILE:dp8382x_cli_fixture>")
  add_test(NAME cli_pin_transcript COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/pin_transcript.py" "$<TARGET_FILE:dp8382x_cli_fixture>")
  add_test(NAME cli_selftest_transcript COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/selftest_transcript.py" "$<TARGET_FILE:dp8382x_cli_fixture>")
  set_tests_properties(cli_fixture_transcript cli_diagnostic_transcript cli_control_transcript
    cli_pin_transcript cli_selftest_transcript PROPERTIES TIMEOUT 30)
else()
  message(STATUS "Python unavailable: CLI subprocess transcript check not registered")
endif()

foreach(suite IN ITEMS line_boundaries atomic_records writer_backpressure)
  add_test(NAME cli_console_${suite} COMMAND dp8382x_console_tests ${suite})
  set_tests_properties(cli_console_${suite} PROPERTIES TIMEOUT 15)
endforeach()
