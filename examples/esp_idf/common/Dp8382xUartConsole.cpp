/// @file Dp8382xUartConsole.cpp
/// @brief Pinned ESP-IDF UART0 pump. It never accesses application or PHY state.
#include "Dp8382xExampleConsole.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <array>
#include <atomic>

#if !CONFIG_LOG_VERSION_1 || !CONFIG_ESP_CONSOLE_UART_DEFAULT || !CONFIG_ESP_CONSOLE_SECONDARY_NONE
#error "The example console requires ESP log V1 and primary UART0 only"
#endif

namespace DP8382x::Cli {
namespace {
constexpr uart_port_t PORT = UART_NUM_0;
constexpr int RX_BYTES = 1024;
constexpr unsigned EVENT_COUNT = 16;
constexpr std::size_t READ_BYTES = 128;
QueueHandle_t events{nullptr};
std::atomic<NativeConsole*> logQueues{nullptr};

int logVprintf(const char* format, std::va_list args) {
  auto* queues = logQueues.load(std::memory_order_acquire);
  return queues ? queues->vprintf(format, args) : 0;
}

std::size_t writeUart(void*, const char* bytes, std::size_t length) {
  // This pump is the sole caller of the installed driver's TX path. The
  // pinned SDK briefly takes its private TX mutex, then accepts FIFO space.
  const auto written = uart_tx_chars(PORT, bytes, static_cast<uint32_t>(length));
  return written > 0 ? static_cast<std::size_t>(written) : 0;
}

bool inputGap() {
  const auto pending = uxQueueMessagesWaiting(events);
  bool lost = pending >= EVENT_COUNT;
  for (unsigned i = 0; i < pending && i < EVENT_COUNT; ++i) {
    uart_event_t event{};
    if (xQueueReceive(events, &event, 0) != pdTRUE) break;
    lost |= event.type == UART_FIFO_OVF || event.type == UART_BUFFER_FULL ||
        event.type == UART_FRAME_ERR || event.type == UART_PARITY_ERR || event.type == UART_BREAK;
  }
  return lost;
}

void pump(void* context) {
  auto& queues = *static_cast<NativeConsole*>(context);
  std::array<uint8_t, READ_BYTES> bytes{};
  for (;;) {
    bool lost = inputGap();
    const int count = lost ? 0 : uart_read_bytes(PORT, bytes.data(), bytes.size(), 0);
    // Check after reading too. A gap makes the just-read bytes unauditable.
    lost = inputGap() || lost || count < 0;
    if (lost) {
      (void)uart_flush_input(PORT);
      queues.noteInputLoss(count > 0 ? static_cast<uint32_t>(count) : 1U);
    } else if (count > 0) {
      queues.offerInput({reinterpret_cast<const char*>(bytes.data()), static_cast<std::size_t>(count)});
    }
    queues.drain(nullptr, writeUart);
    vTaskDelay(1);
  }
}
} // namespace

esp_err_t startUartConsole(NativeConsole& queues) {
  if (events) return ESP_ERR_INVALID_STATE;
  auto status = uart_driver_install(PORT, RX_BYTES, 0, EVENT_COUNT, &events, 0);
  if (status != ESP_OK) return status;
  if (xTaskCreate(pump, "dp8382x-console", 4096, &queues, 1, nullptr) != pdPASS) {
    uart_driver_delete(PORT);
    events = nullptr;
    return ESP_ERR_NO_MEM;
  }
  logQueues.store(&queues, std::memory_order_release);
  esp_log_set_vprintf(logVprintf);
  return ESP_OK;
}
} // namespace DP8382x::Cli
