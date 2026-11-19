get_filename_component(DP8382X_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
get_filename_component(DP8382X_COMPONENT_NAME "${DP8382X_SOURCE_DIR}" NAME)
idf_component_register(SRCS "main.cpp" "PhyOwner.cpp"
  INCLUDE_DIRS "." "../../basic/main"
  REQUIRES ${DP8382X_COMPONENT_NAME} esp_eth esp_event esp_timer esp_driver_gpio freertos)
target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_17)
set(identity "${CMAKE_BINARY_DIR}/owner_identity/ExampleIdentity.h")
add_custom_target(dp8382x_owner_identity
  COMMAND "${CMAKE_COMMAND}" "-DSOURCE_DIR=${DP8382X_SOURCE_DIR}" "-DOUTPUT_FILE=${identity}"
    -P "${CMAKE_CURRENT_LIST_DIR}/../../basic/main/BuildIdentity.cmake"
  BYPRODUCTS "${identity}" VERBATIM)
add_dependencies(${COMPONENT_LIB} dp8382x_owner_identity)
target_include_directories(${COMPONENT_LIB} PRIVATE "${CMAKE_BINARY_DIR}/owner_identity")
