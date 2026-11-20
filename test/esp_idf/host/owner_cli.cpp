#include "InstrumentedMac.h"

#include <cassert>
#include <iostream>
#include <memory>
#include <string>

#define DP8382X_OWNER_EXAMPLE_TEST
#include "../../../examples/esp_idf/owner/main/main.cpp"

namespace {
std::unique_ptr<Host::InstrumentedMac> fixtureMac;
}

extern "C" {
const char* esp_err_to_name(esp_err_t error) { return error == ESP_OK ? "ESP_OK" : "SDK_ERROR"; }
esp_err_t esp_event_loop_create_default(void) { return ESP_OK; }
esp_err_t esp_event_loop_delete_default(void) { return ESP_OK; }
esp_err_t gpio_set_level(gpio_num_t, uint32_t) { return ESP_OK; }
esp_err_t gpio_set_direction(gpio_num_t, gpio_mode_t) { return ESP_OK; }
esp_eth_mac_t* esp_eth_mac_new_esp32(const eth_esp32_emac_config_t*, const eth_mac_config_t*) {
    fixtureMac = std::make_unique<Host::InstrumentedMac>();
    return &fixtureMac->native;
}
}

namespace {
struct Transcript {
    std::string bytes;
    uint64_t lastEvidenceUs{0};
    static std::size_t write(void* context, const char* bytes, std::size_t size) {
        static_cast<Transcript*>(context)->bytes.append(bytes, size);
        return size;
    }
    void tick() {
        serviceIteration(lastEvidenceUs);
        console.queues.drain(this, write);
        // SDK timer turns are explicit in these tests so passive and denied
        // commands have frame deltas attributable to the command alone.
        Host::state().nowUs += 1000U;
    }
    void submit(std::string_view line) { assert(console.queues.offerInput(line) == line.size()); }
    void consume() {
        for (unsigned i = 0; i < 256U; ++i) {
            tick();
            if (!console.processor.outputPending() && !console.processor.workPending() &&
                !console.queues.stats().queuedOutput && !console.queues.stats().queuedInput) return;
        }
        assert(false && "owner CLI did not drain");
    }
    void expect(std::string_view command, std::string_view status) const {
        std::string_view remaining(bytes);
        const auto name = std::string(command) + ' ';
        const auto expected = "state=terminal primary=" + std::string(status) + ' ';
        while (!remaining.empty()) {
            const auto end = remaining.find('\n');
            if (end == std::string_view::npos) break;
            const auto line = remaining.substr(0, end);
            if (line.substr(0, 9) == "@DP8382X " && line.find(name) != std::string_view::npos &&
                line.find(expected) != std::string_view::npos) return;
            remaining.remove_prefix(end + 1);
        }
        std::cerr << bytes;
        assert(false && "unexpected raw-owner CLI terminal status");
    }
    void command(std::string_view line, std::string_view name, std::string_view status) {
        bytes.clear();
        submit(line);
        consume();
        expect(name, status);
    }
};

void integration() {
    Host::reset();
    Cli::Identity identity;
    identity.boot = 47;
    identity.backend = "p4-raw-host";
    identity.console = "uart0";
    identity.baud = 115200;
    assert(attachConsole(identity));
    assert(Host::state().calls.empty());
    testTrafficEnabled = true; // The fake MAC provides the opt-in image's source address.
    runBoard();
    assert(driver && phy && fixtureMac && Host::isLive(driver));
    Host::tickTimers();
    Transcript transcript;
    transcript.tick();
    assert(owner.snapshot().phase == OwnerPhase::READY);

    auto before = fixtureMac->frames.size();
    transcript.command("application admit\n", "cmd=application.admit", "OK");
    assert(owner.snapshot().phase == OwnerPhase::ACTIVE);
    transcript.command("configuration desired\n", "cmd=configuration.desired", "OK");
    transcript.command("registers bmcr 0x1000 0\n", "cmd=registers.bmcr", "BUSY");
    transcript.command("application stop\n", "cmd=application.stop", "BUSY");
    testTrafficEnabled = false;
    transcript.command("application send\n", "cmd=application.send", "UNSUPPORTED");
    testTrafficEnabled = true;
    transcript.command("application send\n", "cmd=application.send", "OK");
    assert(txCounts.queuedToDma == 0); // Queue admission had no inline TX.
    Host::state().nowUs = transcript.lastEvidenceUs + 100000U;
    transcript.tick();
    assert(txCounts.queuedToDma == 1);
    assert(fixtureMac->frames.size() == before);

    transcript.command("application release\n", "cmd=application.release", "OK");
    transcript.command("application stop\n", "cmd=application.stop", "OK");
    assert(owner.snapshot().phase == OwnerPhase::MAINTENANCE && maintenanceReady);

    transcript.bytes.clear();
    transcript.submit("recovery reset pcs\n");
    transcript.tick();
    OperationSnapshot operation;
    assert(phy->operationSnapshot(operation).ok() && operation.state == OperationState::RUNNING);
    before = fixtureMac->frames.size();
    transcript.submit("application start\n");
    transcript.submit("owner cancel " + std::to_string(operation.token.value) + "\n");
    transcript.consume();
    transcript.expect("cmd=application.start", "BUSY");
    transcript.expect("cmd=recovery.reset", "CANCELLED");
    assert(maintenanceReady && fixtureMac->frames.size() == before);

    transcript.command("configuration apply an 8 none auto 1\n", "cmd=configuration.apply", "OK");
    transcript.command("application start\n", "cmd=application.start", "OK");
    assert(owner.snapshot().phase == OwnerPhase::WAITING && !maintenanceReady);
    transcript.command("application stop\n", "cmd=application.stop", "OK");
    transcript.command("connection end\n", "cmd=connection.end", "UNSUPPORTED");
    assert(retired && !maintenanceReady && fixtureMac->alive && Host::isLive(driver));
    Host::reset();
}
} // namespace

int main() { integration(); }
