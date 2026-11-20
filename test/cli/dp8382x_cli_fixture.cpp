// Host process for byte-oriented CLI tests. All MDIO runs on a simulated bus.
#include "../../examples/common/Dp8382xCliCore.h"

#include <array>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <limits>
#include <optional>
#include <string_view>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

namespace {
using namespace ::DP8382x;
using namespace ::DP8382x::Cli;
using Driver = ::DP8382x::DP8382x;

enum class Partner { DOWN, PENDING, FULL100, HALF10 };
enum class MaintenancePolicy { ALLOWED, ACTIVE, UNSUPPORTED };
constexpr uint8_t PHY_ADDRESS = 1;
struct Options {
  const char* identity{"unknown"};
  Partner partner{Partner::FULL100};
  uint64_t failAfter{std::numeric_limits<uint64_t>::max()};
  uint64_t rebootAfter{0};
  MaintenancePolicy maintenance{MaintenancePolicy::ALLOWED};
  bool partialRecord{false};
  bool holdReset{false};
  bool regressHard{false};
  bool traceResets{false};
};

bool decimal(std::string_view input, uint64_t& output) {
  if (input.empty()) return false;
  const auto converted = std::from_chars(input.data(), input.data() + input.size(), output);
  return converted.ec == std::errc{} && converted.ptr == input.data() + input.size();
}

bool parseOptions(int argc, char** argv, Options& options) {
  for (int i = 1; i < argc; ++i) {
    const std::string_view option{argv[i]};
    if (option == "--partial-record") { options.partialRecord = true; continue; }
    if (option == "--deny-maintenance") { options.maintenance = MaintenancePolicy::ACTIVE; continue; }
    if (option == "--no-maintenance") { options.maintenance = MaintenancePolicy::UNSUPPORTED; continue; }
    if (option == "--hold-reset") { options.holdReset = true; continue; }
    if (option == "--regress-after-hard") { options.regressHard = true; continue; }
    if (option == "--trace-resets") { options.traceResets = true; continue; }
    if (++i == argc) return false;
    const std::string_view value{argv[i]};
    if (option == "--identity") options.identity = argv[i];
    else if (option == "--partner") {
      if (value == "down") options.partner = Partner::DOWN;
      else if (value == "pending") options.partner = Partner::PENDING;
      else if (value == "100-full") options.partner = Partner::FULL100;
      else if (value == "10-half") options.partner = Partner::HALF10;
      else return false;
    } else if (option == "--fail-after") {
      if (!decimal(value, options.failAfter)) return false;
    } else if (option != "--reboot-after" || !decimal(value, options.rebootAfter) ||
               options.rebootAfter == 0) return false;
  }
  return true;
}

struct Bus {
  struct MmdWord { uint16_t devad, offset, value; };
  std::array<uint16_t, 32> words{};
  std::array<MmdWord, 6> extended{{
      {31, 0x030E, 0x8400}, {31, 0x0467, 0xA567}, {31, 0x0468, 0xA568},
      {3, 0x0014, 0x0002}, {7, 0x003C, 0x0000}, {7, 0x003D, 0x0000},
  }};
  uint16_t selector{0}, offset{0};
  uint64_t nowUs{0}, frames{0}, failAfter{0};
  std::optional<uint64_t> failOnce;
  bool holdReset{false}, regressHard{false}, traceResets{false}, linkLatchedLow{false}, isolated{false};

  void setup(const Options& options) {
    *this = {};
    failAfter = options.failAfter;
    holdReset = options.holdReset;
    regressHard = options.regressHard;
    traceResets = options.traceResets;
    words[0] = 0x3100; // Autonegotiation enabled, no reset in progress.
    words[2] = 0x2000;
    words[3] = 0xA131; // DP83826 enhanced model, documented revision 1.
    words[4] = 0x01E1;
    if (options.partner == Partner::DOWN) return;
    if (options.partner == Partner::PENDING) {
      words[1] = 0x0004; // Link up, with no accepted AN completion or partner word.
      words[16] = 0x0001;
      return;
    }
    words[1] = 0x0024;
    words[5] = options.partner == Partner::FULL100 ? 0x0101 : 0x0021;
    words[6] = 1;
    words[16] = options.partner == Partner::FULL100 ? 0x0015 : 0x0013;
  }

  // Literal register model for the fixture's latch and read-clear assertions.
  uint16_t readDirect(uint8_t address) {
    const auto word = words[address];
    switch (address) {
      case 1:
        if (linkLatchedLow) {
          linkLatchedLow = false;
          return static_cast<uint16_t>(word & 0xFFFBU);
        }
        break;
      case 15: words[address] &= 0xFE0FU; break;
      case 18: case 19: words[address] &= 0x00FFU; break;
      case 20: words[address] &= 0xFF00U; break;
      case 21: words[address] = 0; break;
      default: break;
    }
    return word;
  }

  uint16_t read(uint8_t address) {
    if (address != 14 || (selector & 0xC000U) != 0x4000U) return readDirect(address);
    const auto devad = selector & 0x001FU;
    if (devad == 31 && offset < words.size()) return readDirect(static_cast<uint8_t>(offset));
    for (const auto& reg : extended) {
      if (reg.devad == devad && reg.offset == offset) return reg.value;
    }
    return 0;
  }

  void write(uint8_t address, uint16_t value) {
    if (address == 13) { selector = value; return; }
    if (address != 14) {
      if (address == 0 && value == 0x8000U) {
        if (traceResets) std::fputs("reset 00 8000\n", stderr);
        words[0] = holdReset ? value : uint16_t{0x3100};
      } else if (address == 31 && (value == 0x8000U || value == 0x4000U)) {
        if (traceResets) std::fputs(value == 0x8000U ? "reset 1f 8000\n" : "reset 1f 4000\n", stderr);
        words[31] = holdReset ? value : uint16_t{0};
        if (value == 0x8000U) {
          words[0] = 0x3100;
          words[24] = 0; // Reload the modeled LED register on hard reset.
          extended[0].value = 0x8400;
          if (regressHard) nowUs = 0;
        }
      } else {
        // The AN restart bit clears independently of the modeled partner.
        words[address] = address == 0 ? static_cast<uint16_t>(value & 0xFDFFU) : value;
      }
      return;
    }
    const auto mode = selector & 0xC000U;
    if (mode == 0) { offset = value; return; }
    if (mode != 0x4000U) return;
    const auto devad = selector & 0x001FU;
    if (devad == 31 && offset < words.size()) { words[offset] = value; return; }
    for (auto& reg : extended) {
      if (reg.devad == devad && reg.offset == offset) { reg.value = value; return; }
    }
  }

  static uint64_t clock(void* context) noexcept { return static_cast<Bus*>(context)->nowUs; }

  static BatchResult execute(void* context, const MdioBatch& batch, const IoBudget&) noexcept {
    auto& bus = *static_cast<Bus*>(context);
    BatchResult result{};
    result.releaseStatus = Status::Ok();
    result.controllerReusable = true;
    result.status = Status::Ok();
    if (bus.isolated) {
      result.status = Status::Error(Err::BUSY, "Fixture isolated");
      return result;
    }
    for (uint8_t i = 0; i < batch.frameCount; ++i) {
      const auto& frame = batch.frames[i];
      auto& receipt = result.frames[i];
      receipt.attempted = true;
      bus.nowUs += 2;
      const auto index = bus.frames++;
      if (index >= bus.failAfter || bus.failOnce == index) {
        bus.failOnce.reset();
        receipt.effectPossible = frame.kind == FrameKind::WRITE;
        result.status = Status::Error(Err::MDIO_ERROR, "Simulated MDIO failure", -43);
        break;
      }
      receipt.completed = true;
      if (frame.kind == FrameKind::READ) {
        // Cover both no-response words while leaving address zero legal to scan.
        receipt.value = batch.phyAddress == PHY_ADDRESS ? bus.read(frame.registerAddress) :
            batch.phyAddress == 0 ? uint16_t{0} : uint16_t{0xFFFF};
        receipt.valueValid = true;
      } else if (batch.phyAddress == PHY_ADDRESS) {
        uint16_t value = frame.value;
        if (frame.readIndex != NO_READ_INDEX) {
          value = static_cast<uint16_t>(value |
              (result.frames[frame.readIndex].value & frame.preserveMask));
        }
        bus.write(frame.registerAddress, value);
      }
    }
    return result;
  }
};

class Fixture {
public:
  explicit Fixture(const Options& options) : _options(options) {
    const auto time = static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    _boot = static_cast<uint32_t>(time ^ (time >> 32));
    if (_boot == 0) _boot = 1;
  }

  bool setup() {
    _owner.reset();
    _driver.emplace();
    _bus.setup(_options);
    Config config{};
    config.variant = Variant::DP83826I;
    config.interface = MacInterface::RMII;
    config.phyAddress = PHY_ADDRESS;
    config.transport = {&_bus, Bus::execute, Bus::clock, {8, 4, 1}};
    if (!_driver->bind(config).ok()) return false;

    // Prime the passive views through the real cooperative core and one live
    // link read. An injected MDIO error leaves the actual core failure evidence.
    OperationToken token{};
    const OperationBudget budget{{1000000, 128}, 1000, 8};
    if (!_driver->startInitialize({ResetKind::NONE}, budget, token).inProgress()) return false;
    Status status{Err::IN_PROGRESS, 0, "Pending"};
    for (unsigned i = 0; i < 128 && status.inProgress(); ++i) {
      status = _driver->pollOperation(token);
      if (status.inProgress()) _bus.nowUs += TICK_US;
    }
    if (status.inProgress()) return false;
    OperationResult result{};
    if (!_driver->takeResult(token, result).ok()) return false;
    if (result.status.ok()) {
      LinkSnapshot link{};
      (void)_driver->readLinkStatus(link, {1000000, 10});
    }

    Identity identity{};
    identity.boot = _boot;
    identity.firmware = _options.identity;
    identity.target = "host";
    identity.backend = "sim-mdio";
    identity.clock = "simulated";
    identity.variant = config.variant;
    identity.interface = config.interface;
    identity.phyAddress = config.phyAddress;
    _owner.emplace(*_driver, maintenance, this, externalDisposition);
    _processor.emplace();
    return _processor->setup({this, write, readCached, clock, execute}, identity);
  }

  bool receive(char byte) {
    if (_skipControlLf) {
      _skipControlLf = false;
      if (byte == '\n') return true;
    }
    const bool boundary = byte == '\r' || byte == '\n';
    const bool candidate = _controlSize != 0 || (_lineStart && byte == '$');
    _lineStart = boundary;
    if (candidate && boundary) {
      const auto line = std::string_view(_control.data(), _controlSize);
      if (control(line)) {
        _controlSize = 0;
        _skipControlLf = byte == '\r';
        return !_failed;
      }
    } else if (candidate && _controlSize < _control.size()) {
      _control[_controlSize++] = byte;
      return true;
    }
    for (std::size_t i = 0; i < _controlSize; ++i) {
      if (!stage(_control[i])) return false;
    }
    _controlSize = 0;
    return stage(byte);
  }

  bool finish() {
    // An unrecognized, unterminated control candidate remains ordinary input.
    for (std::size_t i = 0; i < _controlSize; ++i) {
      if (!stage(_control[i])) return false;
    }
    _controlSize = 0;
    if (_rebootPending && !reboot()) return false;
    if (!pump(AUTO_TICKS)) return false;
    if (_rebootPending && (!reboot() || !pump(AUTO_TICKS))) return false;
    return _inputSize == 0 && !_processor->outputPending() && !_processor->workPending();
  }

  bool startOutput() { return pump(AUTO_TICKS); }

private:
  static constexpr uint64_t TICK_US = 1000;
  static constexpr uint64_t AUTO_TICKS = 8192; // Covers the 5 s profile deadline plus final output.
  static constexpr uint64_t CONTROL_LIMIT = 4096;
  static constexpr std::string_view PREFIX = "@DP8382X ";
  Options _options;
  Bus _bus{};
  std::optional<Driver> _driver;
  std::optional<CoreOwner> _owner;
  std::optional<Processor> _processor;
  std::array<char, 4096> _input{};
  std::array<char, 64> _control{};
  std::size_t _inputHead{0}, _inputSize{0}, _controlSize{0}, _prefixAt{0};
  uint32_t _boot{0};
  uint64_t _lines{0}, _outputStall{0};
  bool _lineStart{true}, _skipControlLf{false}, _previousCr{false};
  bool _paused{false}, _rebootPending{false}, _rebooted{false}, _failed{false};
  bool _outputAtStart{true}, _cutOutput{false};

  bool control(std::string_view line) {
    if (line == "$pause") _paused = true;
    else if (line == "$latch-link-low") _bus.linkLatchedLow = true;
    else if (line == "$isolate") _bus.isolated = true;
    else if (line == "$inject-events") {
      _bus.words[18] |= 0xA500U;
      _bus.words[19] |= 0x5A00U;
    } else if (line.substr(0, 11) == "$fail-next ") {
      uint64_t offset{};
      if (!decimal(line.substr(11), offset) || offset > CONTROL_LIMIT) return false;
      _bus.failOnce = _bus.frames + offset;
    } else if (line == "$resume") {
      _paused = false;
      _failed = !pump(AUTO_TICKS);
    } else if (line.substr(0, 6) == "$tick ") {
      uint64_t ticks{};
      if (!decimal(line.substr(6), ticks) || ticks > CONTROL_LIMIT) return false;
      _failed = !pump(ticks, true);
    } else if (line == "$before-quiet-floor" || line == "$at-quiet-floor") {
      const auto state = _driver->operationSnapshot();
      if (!_paused || !state.busQuietValid || state.busQuietUntilUs == 0) return false;
      const uint64_t target = state.busQuietUntilUs - (line == "$before-quiet-floor" ? 1U : 0U);
      if (target < _bus.nowUs) return false;
      _bus.nowUs = target;
      _failed = !pump(1, true, 0);
    } else if (line.substr(0, 14) == "$output-stall ") {
      uint64_t calls{};
      if (!decimal(line.substr(14), calls) || calls > CONTROL_LIMIT) return false;
      _outputStall = calls;
    } else return false;
    return true;
  }

  bool stage(char byte) {
    if (_inputSize == _input.size()) return false;
    _input[(_inputHead + _inputSize) % _input.size()] = byte;
    ++_inputSize;
    if (!acceptInput()) return false;
    if (_rebootPending && _inputSize == 0) return true;
    if (!_paused && (byte == '\r' || byte == '\n' || _inputSize != 0)) return pump(AUTO_TICKS);
    return true;
  }

  bool acceptInput() {
    for (std::size_t i = 0; i < _input.size() && _inputSize != 0; ++i) {
      const char byte = _input[_inputHead];
      const bool rebootLf = _rebootPending && byte == '\n';
      if (_rebootPending && !rebootLf && !reboot()) return false;
      const auto accepted = _processor->feed(std::string_view(&byte, 1));
      // The LF paired with the request's CR still belongs to the old processor.
      // If its output is full, reset loses that byte along with the old result.
      if (accepted == 0 && !rebootLf) return true;
      _inputHead = (_inputHead + 1) % _input.size();
      --_inputSize;
      if (rebootLf) {
        if (!reboot()) return false;
        continue;
      }
      const bool boundary = byte == '\r' || (byte == '\n' && !_previousCr);
      _previousCr = byte == '\r';
      if (boundary && ++_lines == _options.rebootAfter && !_rebooted) {
        _rebootPending = byte == '\r';
        if (!_rebootPending && !reboot()) return false;
      }
    }
    return true;
  }

  bool reboot() {
    _rebootPending = false;
    _rebooted = true;
    _previousCr = false;
    if (++_boot == 0) ++_boot;
    return setup();
  }

  bool pump(uint64_t limit, bool explicitTicks = false, uint64_t stepUs = TICK_US) {
    for (uint64_t i = 0; i < limit; ++i) {
      if (!explicitTicks && _inputSize == 0 && !_processor->outputPending() &&
          !_processor->workPending()) break;
      _bus.nowUs += stepUs;
      if (!_rebootPending) _processor->tick();
      if (_failed || !acceptInput()) return false;
    }
    return true;
  }

  static uint64_t clock(void* context) noexcept { return static_cast<Fixture*>(context)->_bus.nowUs; }

  static Status readCached(void* context, SnapshotKind kind, CachedSnapshot& output) noexcept {
    return static_cast<Fixture*>(context)->_owner->readCached(kind, output);
  }

  static Status execute(void* context, const Request& request, Receipt& receipt) {
    return static_cast<Fixture*>(context)->_owner->execute(request, receipt);
  }

  static Status maintenance(void* context) noexcept {
    switch (static_cast<Fixture*>(context)->_options.maintenance) {
      case MaintenancePolicy::ACTIVE:
        return Status::Error(Err::BUSY, "Simulated application session active",
                             static_cast<int32_t>(BusyDetail::OWNER_ADMISSION));
      case MaintenancePolicy::UNSUPPORTED:
        return Status::Error(Err::UNSUPPORTED, "Fixture application has no maintenance mode");
      default: return Status::Ok();
    }
  }

  static bool externalDisposition(void* context) noexcept {
    return static_cast<Fixture*>(context)->_bus.isolated;
  }

  static std::size_t write(void* context, const char* bytes, std::size_t size) noexcept {
    auto& fixture = *static_cast<Fixture*>(context);
    if (fixture._outputStall != 0) {
      --fixture._outputStall;
      return 0;
    }
    std::size_t length = size;
    if (fixture._options.partialRecord) {
      if (fixture._cutOutput) return size;
      for (std::size_t i = 0; i < size; ++i) {
        const char byte = bytes[i];
        if (fixture._outputAtStart && byte == PREFIX[fixture._prefixAt]) {
          if (++fixture._prefixAt == PREFIX.size()) {
            length = i + 1;
            fixture._cutOutput = true;
            break;
          }
        } else {
          fixture._prefixAt = 0;
          fixture._outputAtStart = byte == '\n';
        }
      }
    }
    const auto written = std::fwrite(bytes, 1, length, stdout);
    if (written != length) fixture._failed = true;
    return written == length ? size : written;
  }
};
} // namespace

int main(int argc, char** argv) {
  constexpr const char* usage =
      "Usage: dp8382x_cli_fixture [--identity TOKEN] [--partner down|pending|100-full|10-half] "
      "[--fail-after N] [--reboot-after N] [--partial-record] [--deny-maintenance|--no-maintenance] "
      "[--hold-reset] [--regress-after-hard] [--trace-resets]\n";
  if (argc == 2 && std::string_view(argv[1]) == "--help") {
    std::fputs(usage, stdout);
    return 0;
  }
  Options options{};
  if (!parseOptions(argc, argv, options)) {
    std::fputs(usage, stderr);
    return 2;
  }
#ifdef _WIN32
  if (_setmode(_fileno(stdin), _O_BINARY) == -1 || _setmode(_fileno(stdout), _O_BINARY) == -1) return 1;
#endif
  if (std::setvbuf(stdout, nullptr, _IONBF, 0) != 0) return 1;
  Fixture fixture(options);
  if (!fixture.setup()) {
    std::fputs("Fixture setup failed.\n", stderr);
    return 2;
  }
  if (!fixture.startOutput()) return 1;
  for (int byte; (byte = std::getc(stdin)) != EOF;) {
    if (!fixture.receive(static_cast<char>(static_cast<unsigned char>(byte)))) {
      std::fputs("Fixture input capacity or output failed.\n", stderr);
      return 1;
    }
  }
  if (std::ferror(stdin) || !fixture.finish()) {
    std::fputs("Fixture input or final output failed.\n", stderr);
    return 1;
  }
  return 0;
}
