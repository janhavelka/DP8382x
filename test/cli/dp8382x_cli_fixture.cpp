// Host process for byte-oriented CLI tests. All MDIO runs on a simulated bus.
#include "../../examples/common/Dp8382xCli.h"

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

enum class Partner { DOWN, FULL100, HALF10 };
struct Options {
  const char* identity{"unknown"};
  Partner partner{Partner::FULL100};
  uint64_t failAfter{std::numeric_limits<uint64_t>::max()};
  uint64_t rebootAfter{0};
  bool partialRecord{false};
};

bool decimal(std::string_view input, uint64_t& output) {
  if (input.empty()) return false;
  const auto converted = std::from_chars(input.data(), input.data() + input.size(), output);
  return converted.ec == std::errc{} && converted.ptr == input.data() + input.size();
}

bool parseOptions(int argc, char** argv, Options& options) {
  for (int i = 1; i < argc; ++i) {
    const std::string_view option{argv[i]};
    if (option == "--partial-record") {
      options.partialRecord = true;
      continue;
    }
    if (++i == argc) return false;
    const std::string_view value{argv[i]};
    if (option == "--identity") options.identity = argv[i];
    else if (option == "--partner") {
      if (value == "down") options.partner = Partner::DOWN;
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
  std::array<uint16_t, 32> words{};
  uint16_t selector{0}, offset{0}, vod{0x8400};
  uint64_t nowUs{0}, frames{0}, failAfter{0};

  void setup(Partner partner, uint64_t fail) {
    *this = {};
    failAfter = fail;
    words[0] = 0x3100; // Autonegotiation enabled, no reset in progress.
    words[2] = 0x2000;
    words[3] = 0xA131; // DP83826 enhanced model, documented revision 1.
    words[4] = 0x01E1;
    if (partner == Partner::DOWN) return;
    words[1] = 0x0024;
    words[5] = partner == Partner::FULL100 ? 0x0101 : 0x0021;
    words[6] = 1;
    words[16] = partner == Partner::FULL100 ? 0x0015 : 0x0013;
  }

  uint16_t read(uint8_t address) const {
    if (address == 14 && selector == 0x401F && offset == 0x030E) return vod;
    return words[address];
  }

  void write(uint8_t address, uint16_t value) {
    if (address == 13) selector = value;
    else if (address == 14 && selector == 0x001F) offset = value;
    else if (address == 14 && selector == 0x401F && offset == 0x030E) vod = value;
    else words[address] = value;
  }

  static uint64_t clock(void* context) noexcept { return static_cast<Bus*>(context)->nowUs; }

  static BatchResult execute(void* context, const MdioBatch& batch, const IoBudget&) noexcept {
    auto& bus = *static_cast<Bus*>(context);
    BatchResult result{};
    result.releaseStatus = Status::Ok();
    result.controllerReusable = true;
    result.status = Status::Ok();
    for (uint8_t i = 0; i < batch.frameCount; ++i) {
      const auto& frame = batch.frames[i];
      auto& receipt = result.frames[i];
      receipt.attempted = true;
      bus.nowUs += 2;
      if (bus.frames++ >= bus.failAfter) {
        result.status = Status::Error(Err::MDIO_ERROR, "Simulated MDIO failure", -43);
        break;
      }
      receipt.completed = true;
      if (frame.kind == FrameKind::READ) {
        receipt.value = bus.read(frame.registerAddress);
        receipt.valueValid = true;
      } else {
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
    _driver.emplace();
    _bus.setup(_options.partner, _options.failAfter);
    Config config{};
    config.variant = Variant::DP83826I;
    config.interface = MacInterface::RMII;
    config.phyAddress = 1;
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
    _processor.emplace();
    return _processor->setup({this, write, readCached, clock}, identity);
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
    if (!pump(MAX_TICKS)) return false;
    if (_rebootPending && (!reboot() || !pump(MAX_TICKS))) return false;
    return _inputSize == 0 && !_processor->outputPending();
  }

  bool startOutput() { return pump(MAX_TICKS); }

private:
  static constexpr uint64_t TICK_US = 1000;
  static constexpr uint64_t MAX_TICKS = 4096;
  static constexpr std::string_view PREFIX = "@DP8382X ";
  Options _options;
  Bus _bus{};
  std::optional<Driver> _driver;
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
    else if (line == "$resume") {
      _paused = false;
      _failed = !pump(MAX_TICKS);
    } else if (line.substr(0, 6) == "$tick ") {
      uint64_t ticks{};
      if (!decimal(line.substr(6), ticks) || ticks > MAX_TICKS) return false;
      _failed = !pump(ticks, true);
    } else if (line.substr(0, 14) == "$output-stall ") {
      uint64_t calls{};
      if (!decimal(line.substr(14), calls) || calls > MAX_TICKS) return false;
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
    if (!_paused && (byte == '\r' || byte == '\n' || _inputSize != 0)) return pump(MAX_TICKS);
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

  bool pump(uint64_t limit, bool explicitTicks = false) {
    for (uint64_t i = 0; i < limit; ++i) {
      if (!explicitTicks && _inputSize == 0 && !_processor->outputPending()) break;
      _bus.nowUs += TICK_US;
      if (!_rebootPending) _processor->tick();
      if (_failed || !acceptInput()) return false;
    }
    return true;
  }

  static uint64_t clock(void* context) noexcept { return static_cast<Fixture*>(context)->_bus.nowUs; }

  static Status readCached(void* context, SnapshotKind kind, CachedSnapshot& output) noexcept {
    const auto& driver = *static_cast<Fixture*>(context)->_driver;
    switch (kind) {
      case SnapshotKind::CAPABILITIES: output.capabilities = driver.capabilities(); break;
      case SnapshotKind::HEALTH: output.health = driver.healthSnapshot(); break;
      case SnapshotKind::LINK: output.link = driver.linkSnapshot(); break;
      case SnapshotKind::SETTINGS: output.settings = driver.settingsSnapshot(); break;
      case SnapshotKind::OPERATION: output.operation = driver.operationSnapshot(); break;
      case SnapshotKind::TRANSPORT: output.transport = driver.transportSnapshot(); break;
    }
    return Status::Ok();
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
      "Usage: dp8382x_cli_fixture [--identity TOKEN] [--partner down|100-full|10-half] "
      "[--fail-after N] [--reboot-after N] [--partial-record]\n";
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
