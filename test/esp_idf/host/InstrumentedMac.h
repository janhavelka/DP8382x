#pragma once
#include "Fakes.h"
#include <array>
#include <map>
#include <functional>
#include <type_traits>

namespace Host {
struct Frame { bool write; uint32_t address; uint32_t reg; uint32_t value; };
/** Instrumented public MAC interface; RX pause models the pinned MAC source. */
struct InstrumentedMac {
    esp_eth_mac_t native{};
    esp_eth_mediator_t* mediator = nullptr;
    esp_eth_mediator_t* capturedMediator = nullptr;
    std::array<uint16_t, 32> registers{};
    std::map<uint16_t, uint16_t> extended;
    std::vector<Frame> frames;
    std::map<std::string, esp_err_t> errors;
    std::map<std::string, uint32_t> elapsedUs;
    std::vector<esp_err_t> readResults;
    std::vector<esp_err_t> writeResults;
    std::function<void(const char*)> onCall;
    std::function<void(const Frame&)> onFrame;
    std::vector<eth_speed_t> speedRequests;
    std::vector<eth_duplex_t> duplexRequests;
    std::vector<uint32_t> pauseRequests;
    std::vector<bool> flowRequests;
    bool flowEnabled = false;
    uint16_t selector = 0;
    bool dataMode = false;
    bool alive = true;
    bool rxPaused = false;
    eth_link_t link = ETH_LINK_DOWN;
    explicit InstrumentedMac();
    esp_err_t result(const char* operation);
    void pauseRxAfterMediatorLoad();
    esp_err_t resumeRx();
};
}
