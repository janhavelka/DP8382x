#include "InstrumentedMac.h"
#include <cassert>
#include <cstdlib>
namespace Host {
namespace {
InstrumentedMac& self(esp_eth_mac_t* native) { return *reinterpret_cast<InstrumentedMac*>(native); }
}
InstrumentedMac::InstrumentedMac() {
    registers[2] = 0x2000; registers[3] = 0xA131;
    registers[0] = 0x1000; registers[1] = 0x786D;
    registers[4] = 0x01E1; registers[5] = 0x01E1;
    registers[6] = 0x0001; // Clause 28 partner implements Auto-Negotiation.
    registers[0x10] = 0x0015;
    native.set_mediator = [](esp_eth_mac_t* mac, esp_eth_mediator_t* value) { auto& m = self(mac); m.mediator = value; return m.result("mac.mediator"); };
    native.init = [](esp_eth_mac_t* mac) { return self(mac).result("mac.init"); };
    native.deinit = [](esp_eth_mac_t* mac) { return self(mac).result("mac.deinit"); };
    native.start = [](esp_eth_mac_t* mac) { return self(mac).result("mac.start"); };
    native.stop = [](esp_eth_mac_t* mac) { return self(mac).result("mac.stop"); };
    native.del = [](esp_eth_mac_t* mac) { auto& m = self(mac); m.alive = false; return m.result("mac.del"); };
    native.set_link = [](esp_eth_mac_t* mac, eth_link_t link) { auto& m = self(mac); m.link = link; return m.result(link == ETH_LINK_UP ? "mac.up" : "mac.down"); };
    native.set_speed = [](esp_eth_mac_t* mac, eth_speed_t value) { auto& m = self(mac); m.speedRequests.push_back(value); return m.result("mac.speed"); };
    native.set_duplex = [](esp_eth_mac_t* mac, eth_duplex_t value) { auto& m = self(mac); m.duplexRequests.push_back(value); return m.result("mac.duplex"); };
    native.set_peer_pause_ability = [](esp_eth_mac_t* mac, uint32_t value) { auto& m = self(mac); m.pauseRequests.push_back(value); return m.result("mac.pause"); };
    native.set_addr = [](esp_eth_mac_t* mac, uint8_t*) { return self(mac).result("mac.set_addr"); };
    native.get_addr = [](esp_eth_mac_t* mac, uint8_t* out) { for (unsigned i = 0; i < 6; ++i) out[i] = static_cast<uint8_t>(i); return self(mac).result("mac.get_addr"); };
    native.transmit = [](esp_eth_mac_t* mac, uint8_t*, uint32_t) { return self(mac).result("mac.transmit"); };
    native.set_promiscuous = [](esp_eth_mac_t* mac, bool) { return self(mac).result("mac.promiscuous"); };
    native.enable_flow_ctrl = [](esp_eth_mac_t* mac, bool enabled) {
        auto& m = self(mac); m.flowRequests.push_back(enabled);
        const auto error = m.result("mac.flow");
        if (error == ESP_OK) m.flowEnabled = enabled;
        return error;
    };
    native.read_phy_reg = [](esp_eth_mac_t* mac, uint32_t address, uint32_t reg, uint32_t* out) {
        auto& m = self(mac); const auto error = m.result("mac.read");
        if (error != ESP_OK) return error;
        if (!m.readResults.empty()) {
            const auto scripted = m.readResults.front(); m.readResults.erase(m.readResults.begin());
            if (scripted != ESP_OK) return scripted;
        }
        if (reg > 31 || !out) return ESP_ERR_INVALID_ARG;
        const uint32_t value = reg == 14 && m.dataMode ?
            (m.selector < 32 ? m.registers[m.selector] : m.extended[m.selector]) : m.registers[reg];
        const Frame frame{false, address, reg, value};
        m.frames.push_back(frame); *out = value;
        if (reg == 0x14 || reg == 0x15) m.registers[reg] = 0; // Explicit counter collector model.
        if (m.onFrame) m.onFrame(frame);
        return ESP_OK;
    };
    native.write_phy_reg = [](esp_eth_mac_t* mac, uint32_t address, uint32_t reg, uint32_t value) {
        auto& m = self(mac); const auto error = m.result("mac.write");
        if (error != ESP_OK) return error;
        if (!m.writeResults.empty()) {
            const auto scripted = m.writeResults.front(); m.writeResults.erase(m.writeResults.begin());
            if (scripted != ESP_OK) return scripted;
        }
        if (reg > 31 || value > 0xFFFF) return ESP_ERR_INVALID_ARG;
        m.frames.push_back({true, address, reg, value});
        if (reg == 13) m.dataMode = (value & 0x4000) != 0;
        else if (reg == 14 && !m.dataMode) m.selector = static_cast<uint16_t>(value);
        else if (reg == 14 && m.selector < 32) m.registers[m.selector] = static_cast<uint16_t>(m.selector == 0 ? value & ~0x8200U : value);
        else if (reg == 14) m.extended[m.selector] = static_cast<uint16_t>(value);
        // Terminal fake models self-clearing reset/restart commands, not silicon timing.
        else m.registers[reg] = static_cast<uint16_t>(reg == 0 ? value & ~0x8200U : reg == 31 ? value & ~0xC000U : value);
        const Frame frame{true, address, reg, value};
        if (m.onFrame) m.onFrame(frame);
        if (m.timeoutAfterWriteAtFrame == m.frames.size()) {
            m.timeoutAfterWriteAtFrame = 0;
            return ESP_ERR_TIMEOUT;
        }
        return ESP_OK;
    };
}
esp_err_t InstrumentedMac::result(const char* operation) {
    state().calls.emplace_back(operation);
    if (onCall) onCall(operation);
    state().nowUs += elapsedUs[operation];
    const auto found = errors.find(operation);
    if (found == errors.end()) return ESP_OK;
    const auto value = found->second; errors.erase(found); return value;
}
void InstrumentedMac::pauseRxAfterMediatorLoad() {
    assert(alive && mediator && !rxPaused);
    capturedMediator = mediator; rxPaused = true;
    state().calls.emplace_back("rx.mediator_loaded");
}
esp_err_t InstrumentedMac::resumeRx() {
    assert(alive && rxPaused && capturedMediator);
    state().calls.emplace_back("rx.resume");
    rxPaused = false;
    auto* buffer = static_cast<uint8_t*>(std::calloc(64, 1));
    assert(buffer);
    // No app callback has executed while paused. Use the retained old mediator,
    // as esp_eth_mac_esp.c does between its unsynchronized load and callback.
    return capturedMediator->stack_input_info(capturedMediator, buffer, 64, nullptr);
}
}
