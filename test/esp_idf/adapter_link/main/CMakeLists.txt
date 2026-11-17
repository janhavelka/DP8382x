get_filename_component(DP8382X_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
get_filename_component(DP8382X_COMPONENT_NAME "${DP8382X_SOURCE_DIR}" NAME)
idf_component_register(SRCS "main.cpp"
  REQUIRES ${DP8382X_COMPONENT_NAME} esp_eth esp_event esp_timer esp_driver_gpio freertos)
target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_17)

# Refresh only this application's Git identity on every build, including source
# edits that do not cause CMake to configure again. The core has no Git dependency.
set(DP8382X_FIXTURE_IDENTITY "${CMAKE_BINARY_DIR}/adapter_identity/FixtureIdentity.h")
add_custom_target(dp8382x_fixture_identity
  COMMAND "${CMAKE_COMMAND}" "-DSOURCE_DIR=${DP8382X_SOURCE_DIR}"
    "-DOUTPUT_FILE=${DP8382X_FIXTURE_IDENTITY}"
    -P "${CMAKE_CURRENT_LIST_DIR}/BuildIdentity.cmake"
  BYPRODUCTS "${DP8382X_FIXTURE_IDENTITY}" VERBATIM)
add_dependencies(${COMPONENT_LIB} dp8382x_fixture_identity)
target_include_directories(${COMPONENT_LIB} PRIVATE "${CMAKE_BINARY_DIR}/adapter_identity")
