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
    --compiler "${CMAKE_C_COMPILER}" --cxx "${CMAKE_CXX_COMPILER}"
    --force-include "${host}/platform/host_compat.h")
foreach(include IN LISTS sdk_includes)
  list(APPEND verify_command --include "${include}")
endforeach()
foreach(source IN LISTS sdk_sources)
  list(APPEND verify_command --source "${source}")
endforeach()
list(APPEND verify_command --source "${host}/probe.c")
list(APPEND verify_command --source "${host}/probe.cpp")
foreach(source IN ITEMS Fakes.cpp InstrumentedMac.cpp driver.cpp native.cpp cli44.cpp)
  list(APPEND verify_command --source "${host}/${source}")
endforeach()
list(APPEND verify_command --include "${host}" --source "${repo}/src/esp_idf/Phy.cpp")
list(APPEND verify_command --source "${repo}/src/esp_idf/PhyControl.cpp")
set(verify_command_100hz ${verify_command} --define DP8382X_HOST_FREERTOS_HZ=100)
set(verify_command_333hz ${verify_command} --define DP8382X_HOST_FREERTOS_HZ=333)
execute_process(COMMAND ${verify_command_333hz} RESULT_VARIABLE sdk_verified_333hz)
execute_process(COMMAND ${verify_command_100hz} RESULT_VARIABLE sdk_verified_100hz)
execute_process(COMMAND ${verify_command} RESULT_VARIABLE sdk_verified)
if(NOT sdk_verified EQUAL 0 OR NOT sdk_verified_100hz EQUAL 0 OR NOT sdk_verified_333hz EQUAL 0)
  message(FATAL_ERROR "Pinned SDK file/dependency verification failed")
endif()
add_custom_target(dp8382x_verify_pinned_sdk
    COMMAND ${verify_command} COMMAND ${verify_command_100hz} COMMAND ${verify_command_333hz} VERBATIM)

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

add_library(dp8382x_native_host STATIC "${repo}/src/esp_idf/Phy.cpp" "${repo}/src/esp_idf/PhyControl.cpp")
target_link_libraries(dp8382x_native_host PUBLIC DP8382x dp8382x_idf_host_fakes)
target_compile_options(dp8382x_native_host PRIVATE -Wall -Wextra -Werror -UNDEBUG)
set(owner_sources "${repo}/examples/esp_idf/owner/main/PhyOwner.cpp"
                  "${repo}/examples/esp_idf/owner/main/FramePort.cpp")
add_executable(dp8382x_native_host_tests host/native.cpp ${owner_sources})
target_link_libraries(dp8382x_native_host_tests PRIVATE dp8382x_native_host)
target_compile_options(dp8382x_native_host_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)

add_executable(dp8382x_native_cli_tests host/cli44.cpp "${repo}/examples/common/Dp8382xCli.cpp")
target_link_libraries(dp8382x_native_cli_tests PRIVATE dp8382x_native_host)
target_compile_options(dp8382x_native_cli_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)
foreach(suite IN ITEMS admission reconciliation discovery raw_access cross_entry_boundaries
                       restart_after_raw_observation shared_silence clock_fault
                        link_events lifecycle features feature_admission blocked_poll)
  add_test(NAME native_cli_${suite} COMMAND dp8382x_native_cli_tests ${suite})
  set_tests_properties(native_cli_${suite} PROPERTIES TIMEOUT 30)
endforeach()

add_executable(dp8382x_uart_host_tests host/uart_console.cpp "${repo}/examples/common/Dp8382xCli.cpp")
target_link_libraries(dp8382x_uart_host_tests PRIVATE DP8382x)
target_include_directories(dp8382x_uart_host_tests PRIVATE "${host}/uart" ${sdk_includes}
    "${sdk}/components/esp_driver_uart/include" "${sdk}/components/esp_hal_uart/include"
    "${sdk}/components/soc/esp32p4/register/hw_ver1")
target_compile_definitions(dp8382x_uart_host_tests PRIVATE
    CONFIG_LOG_VERSION_1=1 CONFIG_ESP_CONSOLE_UART_DEFAULT=1 CONFIG_ESP_CONSOLE_SECONDARY_NONE=1)
target_compile_options(dp8382x_uart_host_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)
add_test(NAME native_uart_error_latch COMMAND dp8382x_uart_host_tests error_latch)
add_test(NAME native_uart_verdict COMMAND "${Python3_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/../verify_check_exit.py" "$<TARGET_FILE:dp8382x_uart_host_tests>")
set_tests_properties(native_uart_error_latch PROPERTIES TIMEOUT 15)

# The application lifecycle tests include the actual P4 example translation
# unit with SDK boundary calls injected by the host test. No duplicate owner.
set(example_source "${host}/example.cpp")
set(example_defines CONFIG_ETH_USE_ESP32_EMAC=1 CONFIG_IDF_TARGET_ESP32P4=1)
set(example_includes
    "${sdk}/components/esp_driver_gpio/include"
    "${sdk}/components/esp_hw_support/include"
    "${sdk}/components/esp_hal_gpio/include"
    "${sdk}/components/esp_hal_gpio/esp32p4/include"
    "${sdk}/components/esp_system/include"
    "${repo}/include" "${host}" ${sdk_includes})
list(REMOVE_DUPLICATES example_includes)
set(example_options -Wall -Wextra -Werror -UNDEBUG -std=gnu++17)
add_executable(dp8382x_example_host_tests "${example_source}" "${repo}/examples/common/Dp8382xCli.cpp")
target_link_libraries(dp8382x_example_host_tests PRIVATE dp8382x_native_host)
target_compile_definitions(dp8382x_example_host_tests PRIVATE ${example_defines})
target_include_directories(dp8382x_example_host_tests PRIVATE ${example_includes})
target_compile_options(dp8382x_example_host_tests PRIVATE ${example_options})

set(example_verify "${Python3_EXECUTABLE}" "${host}/verify_sdk.py"
    --sdk "${sdk}" --manifest "${host}/example-sdk-files.json"
    --compiler "${CMAKE_C_COMPILER}" --cxx "${CMAKE_CXX_COMPILER}" --source "${example_source}")
foreach(definition IN LISTS example_defines)
  list(APPEND example_verify --define "${definition}")
endforeach()
foreach(include IN LISTS example_includes)
  list(APPEND example_verify --include "${include}")
endforeach()
set(scan_configs ${CMAKE_CONFIGURATION_TYPES})
if(NOT scan_configs)
  set(scan_configs "${CMAKE_BUILD_TYPE}")
endif()
if(NOT scan_configs)
  set(scan_configs default)
endif()
# Resolve relative compiler flags where the generator compiles example.cpp.
# Ninja invokes it from the top build tree, Make from this target's directory.
set(example_compile_dir "${CMAKE_CURRENT_BINARY_DIR}")
if(CMAKE_GENERATOR MATCHES "Ninja")
  set(example_compile_dir "${CMAKE_BINARY_DIR}")
endif()
foreach(config IN LISTS scan_configs)
  string(TOUPPER "${config}" config_upper)
  separate_arguments(config_options NATIVE_COMMAND "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${config_upper}}")
  set(verify_example_config ${example_verify})
  foreach(option IN LISTS config_options example_options)
    list(APPEND verify_example_config "--option=${option}")
  endforeach()
  execute_process(COMMAND ${verify_example_config} WORKING_DIRECTORY "${example_compile_dir}"
      RESULT_VARIABLE example_verified)
  if(NOT example_verified EQUAL 0)
    message(FATAL_ERROR "Pinned example SDK dependency verification failed (${config})")
  endif()
  list(APPEND verify_example_commands COMMAND ${verify_example_config})
endforeach()
add_custom_target(dp8382x_verify_example_sdk ${verify_example_commands}
    WORKING_DIRECTORY "${example_compile_dir}" VERBATIM)
add_dependencies(dp8382x_example_host_tests dp8382x_verify_example_sdk)
add_test(NAME example_sdk_header_drift
    COMMAND "${Python3_EXECUTABLE}" "${host}/check_example_sdk_drift.py" ${verify_example_config})
set_tests_properties(example_sdk_header_drift PROPERTIES TIMEOUT 30 WORKING_DIRECTORY "${example_compile_dir}")
foreach(suite IN ITEMS board_config gate_closed shared_loop netif_init_failure
    event_loop_failure netif_new_failure mutex_failure mac_new_failure
    factory_failure board_failure board_direction_failure driver_allocation_failure
    install_failure cleanup_failure mac_cleanup_failure
    glue_registration_1 glue_registration_2 glue_registration_3
    glue_registration_4 glue_registration_5
    app_registration_6 app_registration_7 app_registration_8
    attach_partial_failure event_burst_no_console start_stop start_failure
    stop_failure stop_event_failure start_timer_failure stop_timer_failure
    start_mdio_failure queued_expected_stop retired_rx_before_stop_delivery
    retired_rx_after_stop_delivery unexpected_stop quarantined_mutex_retained
    operation_busy_retry operation_result_owed running_stop_required lost_event_retry
    lost_event_unresolved link_evidence_age board_release_failure last_event_retry last_event_retry_on_time
    unexpected_stop_gap negotiating_start emac_address_failure netif_address_mismatch
    netif_address_read_failure live_cleanup_order live_cleanup_denied terminal_delivery_failure
    terminal_busy terminal_snapshot_delivery owner_timeout_delivered running_failures journal_retry journal_unavailable
    cli_network cli_slow_stop
    journal_retry_stop cleanup_journal_unavailable)
  add_test(NAME example_${suite} COMMAND dp8382x_example_host_tests ${suite})
  set_tests_properties(example_${suite} PROPERTIES TIMEOUT 30)
endforeach()
function(add_example_output_test suite)
  add_test(NAME example_${suite} COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../test_example_output.py" --check
      "$<TARGET_FILE:dp8382x_example_host_tests>" "${suite}" ${ARGN})
  set_tests_properties(example_${suite} PROPERTIES TIMEOUT 30)
endfunction()
add_example_output_test(profile_match linkEvidence=CURRENT profile100full=yes)
add_example_output_test(profile_mismatch linkEvidence=CURRENT profile100full=no)
add_example_output_test(profile_aged linkEvidence=AGED profile100full=unknown)
add_example_output_test(profile_busy linkEvidence=BUSY profile100full=unknown nativePhase=BUSY)
add_example_output_test(journal_failed_call "primary stage=start cause=reported core=OK detail=0 SDK=INJECTED_OR_SDK_ERROR")
add_example_output_test(glue_allocation_failure "primary stage=netif glue cause=unreported core=OK detail=0 SDK=ESP_OK")
add_test(NAME example_output_verdict COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../test_example_output.py")
set_tests_properties(example_output_verdict PROPERTIES TIMEOUT 30)

add_executable(dp8382x_owner_cli_host_tests host/owner_cli.cpp
    "${repo}/examples/common/Dp8382xCli.cpp"
    "${repo}/examples/esp_idf/owner/main/PhyOwner.cpp"
    "${repo}/examples/esp_idf/owner/main/FramePort.cpp")
target_link_libraries(dp8382x_owner_cli_host_tests PRIVATE dp8382x_native_host)
target_compile_definitions(dp8382x_owner_cli_host_tests PRIVATE
    CONFIG_ETH_USE_ESP32_EMAC=1 CONFIG_IDF_TARGET_ESP32P4=1)
target_include_directories(dp8382x_owner_cli_host_tests PRIVATE
    "${sdk}/components/esp_driver_gpio/include"
    "${sdk}/components/esp_hw_support/include"
    "${sdk}/components/esp_hal_gpio/include"
    "${sdk}/components/esp_hal_gpio/esp32p4/include"
    "${sdk}/components/esp_system/include")
target_compile_options(dp8382x_owner_cli_host_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)
add_test(NAME owner_cli_integration COMMAND dp8382x_owner_cli_host_tests)
set_tests_properties(owner_cli_integration PROPERTIES TIMEOUT 30)

set(native_suites factory board_install_failures raw_routing raw_rx_retention
    glue_rx_retention stop_failures partial_start_fence notifications
    notification_failures lost_link_event shared_quarantine callback_bounds
    live_tuple_changes incomplete_negotiation pause_projection notification_partial_receipts
    notification_late_retry notification_completion_contention notification_late_event_retry
    board_tick_readiness poll_period_bounds start_completion_contention stop_completion_contention
    forbidden_lifecycle discovery discovery_admission discovery_accounting terminal_event_retry
    installer_contention raw_read_arbitration raw_lock_failures controller_wait_classification raw_boundary_failures
    raw_accepted_write_timeouts rejected_raw_reads forced_lifecycle raw_state_reconciliation
    direct_start_invalidation flow_control flow_control_callback_bounds sdk_control_callback_bounds raw_mutation_arbitration
    raw_mutation_admission collector_partial_evidence uncertain_basic_control
    configuration_admission hard_reset_silence board_respects_initialization_silence
    board_silence_excludes_other_owners failed_board_silence_remains_closed flow_completion_contention
    forced_partner_pause stopping_late_poll foreign_flow_effect start_gate_failures start_callback_ownership
    initialize_failures configuration_failures retire_and_stop_admission board_completion_contention
    late_start_cleanup uninstall_gate_contention stop_failure_receipts stop_without_start
    stop_repairs_lost_down observation_loss p4_busy_before_command dependent_callback_bounds install_reservation
    timer_publication_during_stop stop_before_admission persistent_disconnected_during_stop
    raw_uninstall_during_stop raw_uninstall_during_initialization retire_after_stop_admission_timeout
    native_cleanup_error_domains observation_cable_flap observation_bus_contention observation_miss_bound
    peer_board_observation event_retry_observation board_deadline_domains board_reset_window board_clock_regression board_release_at_deadline
    mdio_elapsed_deadlines cancelled_reset_board_floor
    late_stop_gate raw_uninstall_already_down failed_start_cleanup_admission minimal_cleanup_allowance
    observation_reset_trust peer_board_failure_observation observation_gate_denial
    quarantine_after_batch_unlock board_deadline_before_drain
    observation_publication_admission start_observation_reserve application_recovery application_frames)
foreach(suite IN ITEMS factory configuration_admission forced_lifecycle initialize_failures configuration_failures)
  add_test(NAME e_native_${suite} COMMAND dp8382x_native_host_tests ${suite} --dp83826e)
  set_tests_properties(e_native_${suite} PROPERTIES TIMEOUT 30)
endforeach()
foreach(suite IN ITEMS discovery feature_admission)
  add_test(NAME e_native_cli_${suite} COMMAND dp8382x_native_cli_tests ${suite} --dp83826e)
  set_tests_properties(e_native_cli_${suite} PROPERTIES TIMEOUT 30)
endforeach()
foreach(suite IN LISTS native_suites)
  add_test(NAME native_${suite} COMMAND dp8382x_native_host_tests ${suite})
  set_tests_properties(native_${suite} PROPERTIES TIMEOUT 30)
endforeach()

# Compile the same pinned sources and production adapter at each host tick rate.
function(add_native_tick_rate rate)
  set(sdk_target dp8382x_pinned_sdk_${rate}hz)
  set(fakes_target dp8382x_idf_host_fakes_${rate}hz)
  set(adapter_target dp8382x_native_host_${rate}hz)
  set(test_target dp8382x_native_host_${rate}hz_tests)
  add_library(${sdk_target} STATIC ${sdk_sources})
  add_dependencies(${sdk_target} dp8382x_verify_pinned_sdk)
  target_include_directories(${sdk_target} PUBLIC ${sdk_includes})
  target_compile_features(${sdk_target} PRIVATE c_std_17)
  target_compile_definitions(${sdk_target} PUBLIC DP8382X_HOST_FREERTOS_HZ=${rate})
  target_compile_definitions(${sdk_target} PRIVATE free=hostSdkFree calloc=hostSdkCalloc)
  target_compile_options(${sdk_target} PRIVATE
      -include "${host}/platform/host_compat.h" -Wall -Wextra -Werror
      -Wno-pointer-to-int-cast -Wno-unused-parameter
      "$<$<C_COMPILER_ID:GNU>:-Wno-old-style-declaration>")
  add_library(${fakes_target} STATIC host/Fakes.cpp host/InstrumentedMac.cpp)
  target_include_directories(${fakes_target} PUBLIC "${host}")
  target_link_libraries(${fakes_target} PUBLIC ${sdk_target})
  target_compile_features(${fakes_target} PUBLIC cxx_std_17)
  target_compile_options(${fakes_target} PRIVATE -Wall -Wextra -Werror -UNDEBUG)
  add_library(${adapter_target} STATIC "${repo}/src/esp_idf/Phy.cpp" "${repo}/src/esp_idf/PhyControl.cpp")
  target_link_libraries(${adapter_target} PUBLIC DP8382x ${fakes_target})
  target_compile_options(${adapter_target} PRIVATE -Wall -Wextra -Werror -UNDEBUG)
  add_executable(${test_target} host/native.cpp ${owner_sources})
  target_link_libraries(${test_target} PRIVATE ${adapter_target})
  target_compile_options(${test_target} PRIVATE -Wall -Wextra -Werror -UNDEBUG)
  foreach(suite IN LISTS ARGN)
    add_test(NAME native_${suite}_${rate}hz COMMAND ${test_target} ${suite})
    set_tests_properties(native_${suite}_${rate}hz PROPERTIES TIMEOUT 30)
  endforeach()
endfunction()
add_native_tick_rate(100 ${native_suites})
add_native_tick_rate(333 ${native_suites})

foreach(grade IN ITEMS ai ae)
  add_test(NAME ${grade}_native_lifecycle COMMAND dp8382x_native_host_tests a_lifecycle --dp83826${grade})
  set_tests_properties(${grade}_native_lifecycle PROPERTIES TIMEOUT 30)
endforeach()
