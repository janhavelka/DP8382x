#include "Fakes.h"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <unordered_set>
#include "esp_heap_caps.h"

struct esp_timer { esp_timer_create_args_t args; bool active; bool periodic; bool running; };
struct esp_netif_obj { esp_netif_driver_ifconfig_t driver{}; };
struct HostSemaphore { bool held = false; };
namespace {
Host::State data;
std::unordered_set<void*> allocations;
std::vector<esp_timer*> timers;
struct Handler { esp_event_base_t base; int32_t id; esp_event_handler_t callback; void* context; };
std::vector<Handler*> handlers;
std::vector<HostSemaphore*> semaphores;
std::vector<esp_netif_obj*> netifs;
size_t delivered = 0;
esp_err_t consume(esp_err_t& value) { const auto result = value; value = ESP_OK; return result; }
}
namespace Host {
State& state() { return data; }
void reset() {
    for (auto* timer : timers) delete timer;
    timers.clear();
    for (auto* handler : handlers) delete handler;
    handlers.clear();
    for (auto* semaphore : semaphores) delete semaphore;
    semaphores.clear();
    for (auto* netif : netifs) delete netif;
    netifs.clear();
    for (auto* allocation : allocations) std::free(allocation);
    allocations.clear();
    data = State{};
    delivered = 0;
}
bool isLive(const void* allocation) { return allocations.count(const_cast<void*>(allocation)) != 0; }
bool semaphoreHeld(SemaphoreHandle_t semaphore) { return semaphore && semaphore->held; }
void deliverEvents() {
    while (delivered < data.events.size()) {
        auto event = data.events[delivered++];
        for (auto* handler : handlers) {
            if (handler->base == ETH_EVENT && (handler->id == event.id || handler->id == ESP_EVENT_ANY_ID))
                handler->callback(handler->context, ETH_EVENT, event.id, &event.handle);
        }
    }
}
void tickTimers() {
    // Both link timers and adapter fences dispatch on the same SDK task.
    if (std::any_of(timers.begin(), timers.end(), [](const esp_timer* timer) { return timer->running; })) return;
    const auto current = timers;
    for (auto* timer : current) {
        if (!timer->active || timer->running || (!timer->periodic && data.holdOneShot)) continue;
        if (!timer->periodic) timer->active = false;
        timer->running = true;
        timer->args.callback(timer->args.arg);
        timer->running = false;
    }
}
esp_netif_t* makeNetif() { auto* netif = new esp_netif_obj; netifs.push_back(netif); return netif; }
void destroyNetif(esp_netif_t* netif) {
    netifs.erase(std::remove(netifs.begin(), netifs.end(), netif), netifs.end());
    delete netif;
}
}
extern "C" {
ESP_EVENT_DEFINE_BASE(IP_EVENT);
SemaphoreHandle_t xSemaphoreCreateMutex(void) {
    if (data.failSemaphore) { data.failSemaphore = false; return nullptr; }
    auto* semaphore = new HostSemaphore;
    semaphores.push_back(semaphore); return semaphore;
}
BaseType_t xSemaphoreTake(SemaphoreHandle_t semaphore, TickType_t ticks) {
    if (const auto callback = data.beforeSemaphoreTake) callback(semaphore);
    bool accepted = semaphore && !semaphore->held;
    if (data.failSemaphoreTakes) { --data.failSemaphoreTakes; accepted = false; }
    if (semaphore && data.failNextTake == semaphore) { data.failNextTake = nullptr; accepted = false; }
    data.semaphoreEvents.push_back({semaphore, true, accepted, ticks});
    if (!accepted) return pdFALSE;
    semaphore->held = true; return pdTRUE;
}
BaseType_t xSemaphoreGive(SemaphoreHandle_t semaphore) {
    bool accepted = semaphore && semaphore->held;
    if (semaphore && data.failNextGive == semaphore) { data.failNextGive = nullptr; accepted = false; }
    data.semaphoreEvents.push_back({semaphore, false, accepted, 0});
    if (!accepted) return pdFALSE;
    semaphore->held = false;
    if (const auto callback = data.afterSemaphoreGive) callback(semaphore);
    return pdTRUE;
}
void vSemaphoreDelete(SemaphoreHandle_t semaphore) {
    assert(semaphore && !semaphore->held);
    semaphores.erase(std::remove(semaphores.begin(), semaphores.end(), semaphore), semaphores.end());
    delete semaphore;
}
void* hostSdkCalloc(size_t count, size_t size) {
    if (data.failAllocation) { data.failAllocation = false; return nullptr; }
    void* value = std::calloc(count, size);
    if (value) allocations.insert(value);
    return value;
}
void hostSdkFree(void* allocation) {
    if (!allocation) return;
    allocations.erase(allocation);
    ++data.released;
    std::free(allocation);
}
void* heap_caps_calloc(size_t count, size_t size, uint32_t) { return hostSdkCalloc(count, size); }
esp_err_t esp_timer_create(const esp_timer_create_args_t* args, esp_timer_handle_t* out) {
    data.calls.emplace_back("timer.create");
    if (const auto error = consume(data.nextTimerCreateError); error != ESP_OK) return error;
    if (!args || !args->callback || !out) return ESP_ERR_INVALID_ARG;
    auto* timer = new esp_timer{*args, false, false, false};
    timers.push_back(timer); *out = timer; ++data.timerCreates; return ESP_OK;
}
esp_err_t esp_timer_delete(esp_timer_handle_t timer) {
    data.calls.emplace_back("timer.delete");
    if (!timer || timer->active) return ESP_ERR_INVALID_STATE;
    timers.erase(std::remove(timers.begin(), timers.end(), timer), timers.end());
    delete timer; ++data.timerDeletes; return ESP_OK;
}
esp_err_t esp_timer_start_periodic(esp_timer_handle_t timer, uint64_t periodUs) {
    data.calls.emplace_back("timer.periodic");
    if (const auto error = consume(data.nextTimerStartError); error != ESP_OK) return error;
    assert(!timer || std::find(timers.begin(), timers.end(), timer) != timers.end());
    if (!timer || timer->active) return ESP_ERR_INVALID_STATE;
    timer->active = true; timer->periodic = true;
    data.periodicPeriodsUs.push_back(periodUs);
    if (const auto callback = data.onTimerPeriodic) callback();
    return ESP_OK;
}
esp_err_t esp_timer_start_once(esp_timer_handle_t timer, uint64_t) {
    data.calls.emplace_back("timer.once"); ++data.oneShotArms;
    if (const auto error = consume(data.nextTimerStartError); error != ESP_OK) return error;
    if (!timer || timer->active) return ESP_ERR_INVALID_STATE;
    timer->active = true; timer->periodic = false;
    if (const auto callback = data.onTimerOnce) callback();
    return ESP_OK;
}
esp_err_t esp_timer_stop(esp_timer_handle_t timer) {
    data.calls.emplace_back("timer.stop");
    if (const auto error = consume(data.nextTimerStopError); error != ESP_OK) return error;
    // Pinned behavior: stopping a timer that was never armed fails.
    if (!timer || !timer->active) return ESP_ERR_INVALID_STATE;
    timer->active = false; return ESP_OK;
}
bool esp_timer_is_active(esp_timer_handle_t timer) { return timer && (timer->active || timer->running); }
int64_t esp_timer_get_time(void) { return static_cast<int64_t>(data.nowUs); }
void vTaskDelay(TickType_t ticks) {
    const auto before = data.nowUs;
    // A one-tick wait may finish almost immediately just before a tick edge.
    if (ticks) {
        const auto currentTick = before * CONFIG_FREERTOS_HZ / 1000000U;
        data.nowUs = ((currentTick + ticks) * 1000000U + CONFIG_FREERTOS_HZ - 1U) / CONFIG_FREERTOS_HZ;
    }
    data.delays.push_back({ticks, before, data.nowUs});
    Host::tickTimers();
}
TaskHandle_t xTaskGetCurrentTaskHandle(void) { return reinterpret_cast<void*>(data.currentTask); }
esp_err_t esp_event_post(esp_event_base_t, int32_t id, const void* event, size_t size, TickType_t ticks) {
    data.calls.emplace_back("event." + std::to_string(id));
    data.nowUs += data.eventElapsedUs;
    if (const auto callback = data.onEventAttempt) callback(id);
    if (data.eventErrorId == -1 || data.eventErrorId == id) {
        if (const auto error = consume(data.nextEventError); error != ESP_OK) return error;
    }
    if (!event || size != sizeof(esp_eth_handle_t)) return ESP_ERR_INVALID_ARG;
    data.events.push_back({id, *static_cast<const esp_eth_handle_t*>(event), ticks});
    if (const auto callback = data.onEventPost) callback(id);
    return ESP_OK;
}
esp_err_t esp_event_handler_instance_register(esp_event_base_t base, int32_t id, esp_event_handler_t callback, void* arg, esp_event_handler_instance_t* out) {
    auto* handler = new Handler{base, id, callback, arg}; handlers.push_back(handler); *out = handler; return ESP_OK;
}
esp_err_t esp_event_handler_instance_unregister(esp_event_base_t, int32_t, esp_event_handler_instance_t instance) {
    auto* handler = static_cast<Handler*>(instance);
    handlers.erase(std::remove(handlers.begin(), handlers.end(), handler), handlers.end()); delete handler; return ESP_OK;
}
esp_err_t esp_netif_attach(esp_netif_t* netif, esp_netif_iodriver_handle handle) {
    auto* base = static_cast<esp_netif_driver_base_t*>(handle); return base->post_attach(netif, handle);
}
esp_err_t esp_netif_set_driver_config(esp_netif_t* netif, const esp_netif_driver_ifconfig_t* config) { netif->driver = *config; return ESP_OK; }
esp_err_t esp_netif_set_mac(esp_netif_t*, uint8_t*) { return ESP_OK; }
esp_err_t esp_netif_set_link_speed(esp_netif_t*, uint32_t) { return ESP_OK; }
esp_err_t esp_netif_receive(esp_netif_t* netif, void* buffer, size_t, void*) {
    ++data.netifRx; netif->driver.driver_free_rx_buffer(netif->driver.handle, buffer); return ESP_OK;
}
void esp_netif_action_start(void*, esp_event_base_t, int32_t, void*) { data.calls.emplace_back("netif.start"); }
void esp_netif_action_stop(void*, esp_event_base_t, int32_t, void*) { data.calls.emplace_back("netif.stop"); }
void esp_netif_action_connected(void*, esp_event_base_t, int32_t, void*) { data.calls.emplace_back("netif.up"); }
void esp_netif_action_disconnected(void*, esp_event_base_t, int32_t, void*) { data.calls.emplace_back("netif.down"); }
void esp_netif_action_got_ip(void*, esp_event_base_t, int32_t, void*) {}
void _esp_error_check_failed(esp_err_t, const char*, int, const char*, const char*) { std::abort(); }
}
