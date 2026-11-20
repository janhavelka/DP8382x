/// @file Dp8382xExampleConsole.h
/// @brief Shared application-side console binding for the two ESP32-P4 examples.
#pragma once

#include "Dp8382xCliNative.h"
#include "../../common/Dp8382xConsoleService.h"
#include <cstdio>

namespace DP8382x::Cli {

/// @brief One static UART console, processor and borrowed native route.
/// @note Bind, change the borrowed PHY and run service only on the application
/// task. The UART task and SDK log hook use only the shared queues.
struct ExampleConsole {
  NativeConsole queues; ///< Bytes and ordinary lines shared across tasks.
  Processor processor; ///< The application task is its sole caller.
  NativeOwner native; ///< Borrowed route; becomes available after installation.
  bool attached{false}; ///< True after local CLI setup.

  /// @brief Establish local identity and callbacks. Performs no PHY or UART I/O.
  /// @param[in] identity Borrowed immutable firmware and board identification.
  /// @return True on first successful attachment.
  bool attach(const Identity& identity) {
    native.context = this;
    native.write = [](void* context, const char* bytes, std::size_t size) {
      return static_cast<ExampleConsole*>(context)->queues.write(bytes, size);
    };
    native.consoleStats = [](void* context, FeatureFields& fields) {
      const auto stats = static_cast<ExampleConsole*>(context)->queues.stats();
      fields.set(" input_queued=%lu output_queued=%lu lines_queued=%lu input_dropped=%lu input_gaps=%lu"
                 " lines_dropped=%lu lines_truncated=%lu uart_stalls=%lu output_backpressure=%lu",
                 static_cast<unsigned long>(stats.queuedInput), static_cast<unsigned long>(stats.queuedOutput),
                 static_cast<unsigned long>(stats.queuedLogs),
                 static_cast<unsigned long>(stats.droppedInput), static_cast<unsigned long>(stats.inputGaps),
                 static_cast<unsigned long>(stats.droppedLogs), static_cast<unsigned long>(stats.truncatedLogs),
                 static_cast<unsigned long>(stats.stalledWrites), static_cast<unsigned long>(stats.outputBackpressure));
    };
    if (!processor.setup(native.platform(), identity)) return false;
    attached = true;
    return true;
  }

  /// @brief Accept bounded input and tick the shared processor once.
  void service() {
    if (attached) queues.service(processor);
  }

  /// @brief Queue one ordinary status line without waiting for the sink.
  /// @param[in] format A static printf format and matching bounded arguments.
  void print(const char* format, ...) {
    std::va_list args;
    va_start(args, format);
    if (attached) queues.vprintf(format, args);
    else std::vprintf(format, args); // Startup error or standalone host lifecycle test.
    va_end(args);
  }
};

/// @brief Check the hardware slot before starting/stopping application service.
/// @param[in] phy Installed native owner, serialized by the application task.
/// @return BUSY while a result, operation or temporary effects require ownership.
inline Status applicationSlot(EspIdf::Phy& phy) {
  OperationSnapshot snapshot;
  const auto copied = phy.operationSnapshot(snapshot);
  if (!copied.ok()) return copied;
  const auto idle = owner_detail::idleSlot(snapshot);
  if (!idle.ok()) return idle;
  if (snapshot.cleanupOwed || snapshot.temporaryMayBeActive)
    return Status::Error(Err::BUSY, "Restore temporary effects first", static_cast<int32_t>(BusyDetail::CLEANUP_REQUIRED));
  return Status::Ok();
}

/// @brief Map a bounded application SDK call while retaining its exact error.
/// @param[in] error The SDK receipt.
/// @return Local outcome with the original SDK detail on failure.
inline Status applicationSdkStatus(esp_err_t error) {
  if (error == ESP_OK) return Status::Ok();
  if (error == ESP_ERR_TIMEOUT) return Status::Error(Err::OPERATION_TIMEOUT, "Application SDK deadline", error);
  if (error == ESP_ERR_NOT_SUPPORTED) return Status::Error(Err::UNSUPPORTED, "SDK resources retained until restart", error);
  return Status::Error(Err::INVALID_CONFIG, "Application SDK call failed", error);
}

/// @brief Launch the single UART0 pump and install the bounded V1 SDK log hook.
/// @param[in,out] queues Static context, retained for the whole boot.
/// @return ESP_OK when the task and hook own the console; otherwise the setup error.
esp_err_t startUartConsole(NativeConsole& queues);

} // namespace DP8382x::Cli
