#include "../../support/Check.h"
#include "freertos/task.h"

// The SDK UART API is unmodified. Only the task/queue boundary and UART
// effects are simulated, with the pump translation unit compiled below.
struct portMUX_TYPE {};
void portENTER_CRITICAL(portMUX_TYPE* lock);
void portEXIT_CRITICAL(portMUX_TYPE* lock);
#define pdPASS pdTRUE
BaseType_t xTaskCreate(TaskFunction_t task, const char*, uint32_t, void* context,
                       UBaseType_t, TaskHandle_t*);

#include "../../../examples/esp_idf/common/Dp8382xUartConsole.cpp"

#include <algorithm>
#include <cstring>
#include <deque>
#include <string>
#include <utility>

namespace {
struct TurnComplete {};
std::deque<uart_event_t> queue;
std::string input;
portMUX_TYPE selectLock;
uart_select_notif_callback_t selectCallback{nullptr};
TaskFunction_t pumpTask{nullptr};
void* pumpContext{nullptr};
uart_event_type_t eventToDrop{UART_EVENT_MAX};
esp_err_t enableStatus{ESP_OK};
bool selectLocked{false}, frameErrorEnabled{false}, reportDuringRead{false};
unsigned enableCalls{0}, deleteCalls{0}, logInstalls{0};
unsigned queueDepth{0}, rejectedEvents{0}, discarded{0}, reads{0};

void appendEvent(uart_event_type_t type) {
  if (queue.size() >= queueDepth) { ++rejectedEvents; return; }
  uart_event_t event{};
  event.type = type;
  queue.push_back(event);
}

void signalError() {
  BaseType_t wake = pdFALSE;
  CHECK(frameErrorEnabled && selectCallback != nullptr);
  portENTER_CRITICAL(&selectLock);
  selectCallback(UART_NUM_0, UART_SELECT_ERROR_NOTIF, &wake);
  portEXIT_CRITICAL(&selectLock);
}

void pumpTurn(std::string bytes) {
  input = std::move(bytes);
  CHECK(pumpTask != nullptr);
  try {
    pumpTask(pumpContext);
  } catch (const TurnComplete&) {
    return;
  }
  CHECK(false);
}

using namespace DP8382x;
using namespace DP8382x::Cli;
struct Fixture {
  NativeConsole queues;
  Processor cli;
  unsigned writes{0};
  uint32_t lastValue{0};

  Fixture() {
    Identity identity;
    identity.boot = 47;
    CHECK(cli.setup({this, write, cache, clock, execute}, identity));
  }

  static std::size_t write(void* context, const char* bytes, std::size_t size) {
    return static_cast<Fixture*>(context)->queues.write(bytes, size);
  }
  static Status cache(void*, SnapshotKind, CachedSnapshot&) { return Status::Ok(); }
  static uint64_t clock(void*) { return 1; }
  static Status execute(void* context, const Request& request, Receipt&) {
    auto& fixture = *static_cast<Fixture*>(context);
    CHECK(request.action == Action::WRITE_C22 && request.address == 24);
    ++fixture.writes;
    fixture.lastValue = request.value;
    return Status::Ok();
  }

  void turn(std::string bytes) {
    pumpTurn(std::move(bytes));
    queues.service(cli);
  }
};
} // namespace

void portENTER_CRITICAL(portMUX_TYPE* lock) { CHECK(lock == &selectLock && !selectLocked); selectLocked = true; }
void portEXIT_CRITICAL(portMUX_TYPE* lock) { CHECK(lock == &selectLock && selectLocked); selectLocked = false; }

BaseType_t xTaskCreate(TaskFunction_t task, const char*, uint32_t, void* context,
                       UBaseType_t, TaskHandle_t*) {
  pumpTask = task;
  pumpContext = context;
  return pdPASS;
}

extern "C" void vTaskDelay(TickType_t ticks) {
  CHECK(ticks == 1);
  throw TurnComplete{};
}

extern "C" BaseType_t xQueueReceive(QueueHandle_t handle, void* output, TickType_t wait) {
  CHECK(handle == &queue && wait == 0);
  if (eventToDrop != UART_EVENT_MAX) {
    // The ISR fills the last spot and loses the fault event before the pump
    // removes the first queued event. Break and buffer-full are queue-only.
    CHECK(queue.size() + 1U == queueDepth);
    appendEvent(UART_DATA);
    if (eventToDrop == UART_FRAME_ERR) signalError();
    appendEvent(eventToDrop);
    eventToDrop = UART_EVENT_MAX;
  }
  if (queue.empty()) return pdFALSE;
  *static_cast<uart_event_t*>(output) = queue.front();
  queue.pop_front();
  return pdTRUE;
}

extern "C" esp_err_t uart_driver_install(uart_port_t port, int, int, int count, QueueHandle_t* handle, int) {
  CHECK(port == UART_NUM_0 && count == 16 && queueDepth == 0);
  queueDepth = static_cast<unsigned>(count);
  *handle = &queue;
  return ESP_OK;
}

extern "C" esp_err_t uart_driver_delete(uart_port_t port) {
  CHECK(port == UART_NUM_0 && queueDepth != 0);
  ++deleteCalls;
  queueDepth = 0;
  queue.clear();
  selectCallback = nullptr;
  frameErrorEnabled = false;
  return ESP_OK;
}

extern "C" esp_err_t uart_enable_intr_mask(uart_port_t port, uint32_t mask) {
  // UART_INT_ENA bit 3 is the framing-error interrupt on ESP32-P4.
  CHECK(port == UART_NUM_0 && mask == 0x08U && queueDepth != 0);
  CHECK(selectCallback != nullptr && !selectLocked && pumpTask == nullptr);
  ++enableCalls;
  frameErrorEnabled = enableStatus == ESP_OK;
  return enableStatus;
}

extern "C" void uart_set_select_notif_callback(uart_port_t port, uart_select_notif_callback_t callback) {
  CHECK(port == UART_NUM_0 && selectLocked);
  selectCallback = callback;
}

extern "C" portMUX_TYPE* uart_get_selectlock(void) { return &selectLock; }
extern "C" vprintf_like_t esp_log_set_vprintf(vprintf_like_t callback) {
  CHECK(callback);
  ++logInstalls;
  return nullptr;
}

extern "C" int uart_read_bytes(uart_port_t port, void* bytes, uint32_t size, TickType_t wait) {
  CHECK(port == UART_NUM_0 && wait == 0);
  ++reads;
  const auto count = std::min<std::size_t>(size, input.size());
  std::memcpy(bytes, input.data(), count);
  if (reportDuringRead) { reportDuringRead = false; signalError(); }
  return static_cast<int>(count);
}

extern "C" int uart_tx_chars(uart_port_t port, const char*, uint32_t size) {
  CHECK(port == UART_NUM_0 && size <= NativeConsole::DRAIN_BYTES);
  return static_cast<int>(size);
}

extern "C" esp_err_t uart_flush_input(uart_port_t port) {
  CHECK(port == UART_NUM_0);
  ++discarded;
  input.clear();
  return ESP_OK;
}

int main() {
  Fixture fixture;
  enableStatus = ESP_FAIL;
  CHECK(startUartConsole(fixture.queues) == ESP_FAIL);
  CHECK(enableCalls == 1 && deleteCalls == 1 && logInstalls == 0);
  CHECK(pumpTask == nullptr && selectCallback == nullptr);
  enableStatus = ESP_OK;
  CHECK(startUartConsole(fixture.queues) == ESP_OK);
  CHECK(enableCalls == 2 && deleteCalls == 1 && logInstalls == 1 && frameErrorEnabled);

  fixture.turn("registers write c22 24 0x");
  CHECK(fixture.writes == 0);
  for (unsigned i = 1; i < queueDepth; ++i) appendEvent(UART_DATA);
  eventToDrop = UART_FRAME_ERR;
  const auto before = reads;
  fixture.turn("2\n"); // A frame error lost the first digit of intended value 0x12.
  CHECK(rejectedEvents == 1 && discarded == 1 && reads == before);
  CHECK(fixture.queues.stats().inputGaps == 1 && fixture.writes == 0);
  fixture.turn("\n"); // First delimiter after the gap closes the rejected line.

  fixture.turn("registers write c22 24 0x");
  reportDuringRead = true;
  fixture.turn("2\n");
  CHECK(discarded == 2 && fixture.queues.stats().inputGaps == 2 && fixture.writes == 0);
  fixture.turn("\n");

  for (const auto fault : {UART_BREAK, UART_BUFFER_FULL}) {
    fixture.turn("registers write c22 24 0x");
    for (unsigned i = 1; i < queueDepth; ++i) appendEvent(UART_DATA);
    eventToDrop = fault;
    const auto rejected = rejectedEvents;
    const auto gaps = fixture.queues.stats().inputGaps;
    fixture.turn("2\n");
    CHECK(rejectedEvents == rejected + 1 && fixture.queues.stats().inputGaps == gaps + 1);
    CHECK(fixture.writes == 0);
    fixture.turn("\n");
  }

  appendEvent(UART_BREAK); // Break still uses the SDK event queue.
  fixture.turn("registers write c22 24 0x2\n");
  CHECK(discarded == 5 && fixture.writes == 0);
  fixture.turn("\nregisters write c22 24 0x12\n");
  CHECK(fixture.writes == 1 && fixture.lastValue == 0x12);
  return 0;
}
