get_filename_component(DP8382X_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
get_filename_component(DP8382X_COMPONENT_NAME "${DP8382X_SOURCE_DIR}" NAME)
idf_component_register(SRCS "main.cpp"
  REQUIRES ${DP8382X_COMPONENT_NAME} esp_eth esp_event esp_netif esp_timer esp_driver_gpio esp_driver_uart log freertos)
include(${CMAKE_CURRENT_LIST_DIR}/../../common/ConsoleSources.cmake)
target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_17)

set(DP8382X_EXAMPLE_IDENTITY "${CMAKE_BINARY_DIR}/basic_identity/ExampleIdentity.h")
add_custom_target(dp8382x_basic_identity
  COMMAND "${CMAKE_COMMAND}" "-DSOURCE_DIR=${DP8382X_SOURCE_DIR}"
    "-DOUTPUT_FILE=${DP8382X_EXAMPLE_IDENTITY}"
    -P "${CMAKE_CURRENT_LIST_DIR}/BuildIdentity.cmake"
  BYPRODUCTS "${DP8382X_EXAMPLE_IDENTITY}" VERBATIM)
add_dependencies(${COMPONENT_LIB} dp8382x_basic_identity)
target_include_directories(${COMPONENT_LIB} PRIVATE "${CMAKE_BINARY_DIR}/basic_identity")
