cmake_minimum_required(VERSION 3.16)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(DP8382xPinnedIdfTests LANGUAGES C CXX)
  enable_testing()
  set(DP8382X_BUILD_PINNED_IDF_TESTS OFF) # Standalone entry owns this target already.
  add_subdirectory(../.. portable)
endif()

set(DP8382X_PINNED_IDF_PATH "" CACHE PATH "Supplied unmodified ESP-IDF v6.1 checkout")
if(NOT EXISTS "${DP8382X_PINNED_IDF_PATH}/components/esp_eth/src/esp_eth.c")
  message(FATAL_ERROR "Pinned driver tests require DP8382X_PINNED_IDF_PATH pointing to the actual SDK checkout")
endif()
find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
if(NOT CMAKE_C_COMPILER_ID MATCHES "^(GNU|Clang)$" OR NOT CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang)$")
  message(FATAL_ERROR "Pinned driver host tests require GNU or Clang for the unmodified C17 SDK source")
endif()
set(sdk "${DP8382X_PINNED_IDF_PATH}")
set(host "${CMAKE_CURRENT_SOURCE_DIR}/host")
get_filename_component(repo "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(sdk_includes "${host}/platform" "${repo}/include")
foreach(component IN ITEMS esp_eth/include esp_common/include esp_timer/include
    esp_event/include esp_hal_emac/include esp_hw_support/etm/include hal/include
    heap/include log/include esp_rom/include soc/esp32p4/include soc/include esp_netif/include)
  list(APPEND sdk_includes "${sdk}/components/${component}")
endforeach()
set(sdk_sources "${sdk}/components/esp_eth/src/esp_eth.c"
    "${sdk}/components/esp_eth/src/esp_eth_netif_glue.c")
set(verify_command "${Python3_EXECUTABLE}" "${host}/verify_sdk.py"
    --sdk "${sdk}" --manifest "${host}/sdk-files.json"
    --compiler "${CMAKE_C_COMPILER}" --cxx "${CMAKE_CXX_COMPILER}")
foreach(include IN LISTS sdk_includes)
  list(APPEND verify_command --include "${include}")
endforeach()
foreach(source IN LISTS sdk_sources)
  list(APPEND verify_command --source "${source}")
endforeach()
list(APPEND verify_command --source "${host}/probe.c")
list(APPEND verify_command --source "${host}/probe.cpp")
foreach(source IN ITEMS Fakes.cpp InstrumentedMac.cpp driver.cpp native.cpp)
  list(APPEND verify_command --source "${host}/${source}")
endforeach()
list(APPEND verify_command --include "${host}" --source "${repo}/src/esp_idf/Phy.cpp")
execute_process(COMMAND ${verify_command} RESULT_VARIABLE sdk_verified)
if(NOT sdk_verified EQUAL 0)
  message(FATAL_ERROR "Pinned SDK file/dependency verification failed")
endif()
add_custom_target(dp8382x_verify_pinned_sdk COMMAND ${verify_command} VERBATIM)

add_library(dp8382x_pinned_sdk STATIC ${sdk_sources})
add_dependencies(dp8382x_pinned_sdk dp8382x_verify_pinned_sdk)
target_include_directories(dp8382x_pinned_sdk PUBLIC ${sdk_includes})
target_compile_features(dp8382x_pinned_sdk PRIVATE c_std_17)
target_compile_options(dp8382x_pinned_sdk PRIVATE
    -include "${host}/platform/host_compat.h" -Wall -Wextra -Werror
    # Upstream source is 32-bit target code; the host uses wider pointers.
    -Wno-pointer-to-int-cast -Wno-unused-parameter
    "$<$<C_COMPILER_ID:GNU>:-Wno-old-style-declaration>")
# Instrument allocator lifetime without changing any supplied SDK source file.
target_compile_definitions(dp8382x_pinned_sdk PRIVATE free=hostSdkFree calloc=hostSdkCalloc)

add_library(dp8382x_idf_host_fakes STATIC host/Fakes.cpp host/InstrumentedMac.cpp)
target_include_directories(dp8382x_idf_host_fakes PUBLIC "${host}")
target_link_libraries(dp8382x_idf_host_fakes PUBLIC dp8382x_pinned_sdk)
target_compile_features(dp8382x_idf_host_fakes PUBLIC cxx_std_17)
target_compile_options(dp8382x_idf_host_fakes PRIVATE -Wall -Wextra -Werror -UNDEBUG)

add_executable(dp8382x_pinned_driver_tests host/driver.cpp)
target_link_libraries(dp8382x_pinned_driver_tests PRIVATE dp8382x_idf_host_fakes)
target_compile_options(dp8382x_pinned_driver_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)
foreach(suite IN ITEMS install_failures raw_ioctl failed_start stop_events timer raw_rx_lifetime glue_rx_lifetime)
  add_test(NAME pinned_driver_${suite} COMMAND dp8382x_pinned_driver_tests ${suite})
  set_tests_properties(pinned_driver_${suite} PROPERTIES TIMEOUT 30)
endforeach()

add_library(dp8382x_native_host STATIC "${repo}/src/esp_idf/Phy.cpp")
target_link_libraries(dp8382x_native_host PUBLIC DP8382x dp8382x_idf_host_fakes)
target_compile_options(dp8382x_native_host PRIVATE -Wall -Wextra -Werror -UNDEBUG)
add_executable(dp8382x_native_host_tests host/native.cpp)
target_link_libraries(dp8382x_native_host_tests PRIVATE dp8382x_native_host)
target_compile_options(dp8382x_native_host_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)
foreach(suite IN ITEMS factory board_install_failures raw_routing raw_rx_retention glue_rx_retention stop_failures partial_start_fence notifications notification_failures lost_link_event shared_quarantine callback_bounds forbidden_lifecycle discovery discovery_admission terminal_event_retry installer_contention)
  add_test(NAME native_${suite} COMMAND dp8382x_native_host_tests ${suite})
  set_tests_properties(native_${suite} PROPERTIES TIMEOUT 30)
endforeach()
