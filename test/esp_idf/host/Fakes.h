#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include "esp_eth_driver.h"
#include "esp_eth_netif_glue.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_event.h"
#include "freertos/semphr.h"

namespace Host {
struct Event { int32_t id; esp_eth_handle_t handle; TickType_t ticks; };
struct Delay { TickType_t ticks; uint64_t beforeUs; uint64_t afterUs; };
struct SemaphoreEvent { SemaphoreHandle_t handle; bool take; bool accepted; TickType_t ticks; };
struct State {
    std::vector<std::string> calls;
    std::vector<Event> events;
    uint64_t eventElapsedUs = 0;
    std::function<void(int32_t)> onEventPost;
    std::vector<Delay> delays;
    esp_err_t nextEventError = ESP_OK;
    int32_t eventErrorId = -1; // -1 means the next event, otherwise only this ID.
    esp_err_t nextTimerCreateError = ESP_OK;
    esp_err_t nextTimerStartError = ESP_OK;
    esp_err_t nextTimerStopError = ESP_OK;
    bool failAllocation = false;
    bool failSemaphore = false;
    unsigned failSemaphoreTakes = 0;
    SemaphoreHandle_t failNextTake = nullptr;
    SemaphoreHandle_t failNextGive = nullptr;
    std::vector<SemaphoreEvent> semaphoreEvents;
    uintptr_t currentTask = 1;
    std::function<void(SemaphoreHandle_t)> beforeSemaphoreTake;
    std::function<void()> onTimerPeriodic;
    std::function<void()> onTimerOnce;
    std::vector<uint64_t> periodicPeriodsUs;
    bool holdOneShot = false;
    uint64_t nowUs = 0;
    unsigned timerCreates = 0;
    unsigned timerDeletes = 0;
    unsigned oneShotArms = 0;
    unsigned appRx = 0;
    unsigned netifRx = 0;
    unsigned released = 0;
};
State& state();
void reset();
void deliverEvents();
void tickTimers();
bool isLive(const void* allocation);
bool semaphoreHeld(SemaphoreHandle_t semaphore);
esp_netif_t* makeNetif();
void destroyNetif(esp_netif_t* netif);
}

extern "C" {
void hostSdkFree(void* allocation);
void* hostSdkCalloc(size_t count, size_t size);
}
