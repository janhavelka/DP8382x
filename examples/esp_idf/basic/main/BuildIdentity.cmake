# Refresh application Git identity each build; archived sources report unknown.
set(commit "unknown")
set(tree "unknown")
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${SOURCE_DIR}/.git")
  execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${SOURCE_DIR}" rev-parse HEAD
    OUTPUT_VARIABLE observed_commit OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE commit_result ERROR_QUIET TIMEOUT 5)
  execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${SOURCE_DIR}" status --porcelain
    OUTPUT_VARIABLE observed_status OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE status_result ERROR_QUIET TIMEOUT 5)
  if(commit_result EQUAL 0 AND status_result EQUAL 0 AND observed_commit MATCHES "^[0-9a-f]+$")
    set(commit "${observed_commit}")
    set(tree "clean")
    if(NOT observed_status STREQUAL "")
      set(tree "dirty")
    endif()
  endif()
endif()
set(content "#pragma once\n#define EXAMPLE_GIT_COMMIT \"${commit}\"\n#define EXAMPLE_GIT_TREE \"${tree}\"\n")
set(previous "")
if(EXISTS "${OUTPUT_FILE}")
  file(READ "${OUTPUT_FILE}" previous)
endif()
if(NOT content STREQUAL previous)
  get_filename_component(output_directory "${OUTPUT_FILE}" DIRECTORY)
  file(MAKE_DIRECTORY "${output_directory}")
  file(WRITE "${OUTPUT_FILE}" "${content}")
endif()
