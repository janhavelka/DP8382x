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
  Variant variant{Variant::DP83826I};
  Partner partner{Partner::FULL100};
  MacInterface interface{MacInterface::RMII};
  uint64_t failAfter{std::numeric_limits<uint64_t>::max()};
  uint64_t rebootAfter{0};
  MaintenancePolicy maintenance{MaintenancePolicy::ALLOWED};
  bool partialRecord{false};
  bool holdReset{false};
  bool regressHard{false};
  bool traceResets{false};
  bool traceWrites{false};
  bool holdTdr{false};
  bool attestFeatures{false};
  std::string_view campaignPeer{"open"};
  std::string_view campaignMode{};
};

// Literal simulated copper/AN profiles. Inputs are confined to this child.
uint16_t campaignPartner(std::string_view name) {
  if (name == "open") return 0;
  if (name == "an-10-half") return 0x0021;
  if (name == "an-10-full") return 0x0041;
  if (name == "an-100-half") return 0x0081;
  if (name == "an-100-full") return 0x0101;
  if (name == "an-100-full-asym") return 0x0901;
  if (name == "forced-10-half") return 0x0020;
  if (name == "forced-10-full") return 0x0040;
  if (name == "forced-100-half") return 0x0080;
  if (name == "forced-100-full") return 0x0100;
  return 0xFFFF;
}

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
    if (option == "--trace-writes") { options.traceWrites = true; continue; }
    if (option == "--mii") { options.interface = MacInterface::MII; continue; }
    if (option == "--hold-tdr") { options.holdTdr = true; continue; }
    if (option == "--attest-features") { options.attestFeatures = true; continue; }
    if (++i == argc) return false;
    const std::string_view value{argv[i]};
    if (option == "--identity") options.identity = argv[i];
    else if (option == "--variant") {
      if (value == "DP83826I") options.variant = Variant::DP83826I;
      else if (value == "DP83826E") options.variant = Variant::DP83826E;
      else if (value == "DP83826AI") options.variant = Variant::DP83826AI;
      else if (value == "DP83826AE") options.variant = Variant::DP83826AE;
      else return false;
    } else if (option == "--partner") {
      if (value == "down") options.partner = Partner::DOWN;
      else if (value == "pending") options.partner = Partner::PENDING;
      else if (value == "100-full") options.partner = Partner::FULL100;
      else if (value == "10-half") options.partner = Partner::HALF10;
      else return false;
    } else if (option == "--campaign") {
      if (value != "basic" && value != "enhanced") return false;
      options.campaignMode = value;
    } else if (option == "--campaign-peer") {
      if (campaignPartner(value) == 0xFFFFU) return false;
      options.campaignPeer = value;
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
  std::array<uint16_t, 0x0500> vendor{};
  std::array<MmdWord, 5> extended{{
      {3, 0x0001, 0x0000}, {3, 0x0014, 0x0002}, {3, 0x0016, 0x0000},
      {7, 0x003C, 0x0000}, {7, 0x003D, 0x0002},
  }};
  uint16_t selector{0}, offset{0};
  uint8_t bistErrors{0}, bistCaptured{0}, tdrPolls{0};
  uint64_t nowUs{0}, frames{0}, failAfter{0};
  std::optional<uint64_t> failOnce;
  bool holdReset{false}, regressHard{false}, traceResets{false}, linkLatchedLow{false}, isolated{false};
  bool traceWrites{false}, holdTdr{false};
  bool campaign{false}, aGrade{false}, basic{false};
  uint16_t peer{0};
  uint64_t peerReadyUs{0};
  bool peerStartsDuringSample{false};

  // Reload the A control words exercised by the core/link campaigns. The
  // simulated board holds Strap7 and Strap11 low, with RMII strapped as leader.
  void resetAControls() {
    const bool rmii = (vendor[0x0468] & 0x0010U) != 0U;
    words[9] = 0;
    words[10] = basic || (vendor[0x0467] & 2U) == 0U ? 0x0102 : 0x0100;
    words[11] = basic ? 0x000D : 0;
    words[17] = basic ? 0x010B : 0x0108;
    words[23] = rmii ? 0x0061 : 0x0041;
    vendor[0x0117] = 0x8147;
    vendor[0x0131] = 0x0288;
    vendor[0x0302] = 0;
    // SNLS783A tables 8-43/50/75..78/86/90. Select external pull-ups
    // for active-low LEDs and model pin 31 in its POR clock role.
    words[24] = 0x0400;
    vendor[0x0025] = 0x0041;
    vendor[0x0303] = 0x0008;
    // Table 8-76 strap sample. The model never programs the disputed 5/6 roles.
    vendor[0x0304] = basic ? 0x0008 : 0x000E;
    vendor[0x0305] = rmii ? 0x0008 : 0x000B;
    vendor[0x0306] = 0x0008;
    vendor[0x0460] = 0x5565;
    vendor[0x0469] = 0;
    vendor[0x0308] = 0x0002;
    vendor[0x0456] = 0x0008;
    vendor[0x04A0] = basic ? 0x1000 : 0x1081;
    vendor[0x04A1] = 0x1000;
    for (unsigned address = 0x04A2; address <= 0x04A7; ++address) vendor[address] = 0;
  }

  void setup(const Options& options) {
    *this = {};
    failAfter = options.failAfter;
    holdReset = options.holdReset;
    regressHard = options.regressHard;
    traceResets = options.traceResets;
    traceWrites = options.traceWrites;
    holdTdr = options.holdTdr;
    campaign = !options.campaignMode.empty();
    peer = campaignPartner(options.campaignPeer);
    words[0] = 0x3100; // Autonegotiation enabled, no reset in progress.
    words[2] = 0x2000;
    basic = options.campaignMode == "basic";
    aGrade = options.variant == Variant::DP83826AI || options.variant == Variant::DP83826AE;
    words[3] = aGrade ? (basic ? 0xA114 : 0xA134) : (basic ? 0xA111 : 0xA131);
    words[4] = 0x01E1;
    words[22] = 0x0100; // BISCR current power-mode indication.
    words[23] = options.interface == MacInterface::MII ? 0x0041 : 0x0061;
    words[25] = 0x8001;
    words[27] = 0x007D;
    words[28] = 0x05EE;
    words[30] = 0x0102; // A pre-existing TDR done bit alone does not prove freshness.
    vendor[0x0025] = 0x0001;
    vendor[0x002A] = 0x7998;
    vendor[0x0117] = 0x8147;
    vendor[0x0131] = 0x2284;
    vendor[0x0170] = 0x4010; // Valid saved TDR tuning for restoration.
    vendor[0x0180] = 6;
    vendor[0x0185] = 68;
    vendor[0x0218] = 0x1234;
    vendor[0x0305] = 0x000B;
    vendor[0x030B] = 0x3C00;
    vendor[0x030C] = 0x0410;
    vendor[0x030E] = 0x8400;
    vendor[0x0456] = 0x0008;
    vendor[0x0460] = 0x5665;
    vendor[0x0461] = 0x0010;
    vendor[0x0467] = 0xA567;
    vendor[0x0468] = 0xA568;
    if (aGrade) {
      // Model a sampled RMII leader or MII. SNLS783A tables 8-13/88/89.
      const bool rmii = options.interface == MacInterface::RMII;
      vendor[0x0467] = rmii ? (basic ? 0x0010 : 0x0102) : 0;
      vendor[0x0468] = rmii ? 0x0010 : 0;
      resetAControls();
    }
    // Inject a pending indication at fixture entry only. HARD reloads the A strap profile.
    vendor[0x04A0] = aGrade ? static_cast<uint16_t>(vendor[0x04A0] | 0x0800U) : 0x1881;
    vendor[0x04A1] = 0x1000;
    vendor[0x04D1] = 0x0080;
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
    if (campaign) {
      if (address == 1U && peerStartsDuringSample) {
        // Become ready between the two bookends of the next link read.
        peerReadyUs = nowUs + 10U;
        peerStartsDuringSample = false;
      }
      const bool an = (words[0] & 0x1000U) != 0U;
      const bool negotiation = an && (peer & 1U) != 0U;
      const auto common = static_cast<uint16_t>(words[4] & peer & 0x01E0U);
      const auto forced = (words[0] & 0x2000U) != 0U ?
          ((words[0] & 0x0100U) != 0U ? 0x0100U : 0x0080U) :
          ((words[0] & 0x0100U) != 0U ? 0x0040U : 0x0020U);
      const bool up = nowUs >= peerReadyUs && (negotiation ? common != 0U : !an && peer == forced);
      const bool complete = up && negotiation;
      const bool speed10 = (common & 0x0180U) == 0U;
      const bool full = (common & 0x0100U) != 0U || (speed10 && (common & 0x0040U) != 0U);
      words[1] = static_cast<uint16_t>((up ? 4U : 0U) | (complete ? 0x20U : 0U));
      words[5] = complete ? peer : 0;
      words[6] = complete ? 1 : 0;
      words[16] = static_cast<uint16_t>((up ? 1U : 0U) | (complete ? 0x10U : 0U) |
                                       (speed10 ? 2U : 0U) | (full ? 4U : 0U));
      // The asymmetric campaign peer raises local RX only with local both.
      words[25] = static_cast<uint16_t>((words[25] & 0xCFFFU) |
          (complete && peer == 0x0901U && (words[4] & 0x0C00U) == 0x0C00U ? 0x2000U : 0U));
    }
    auto word = words[address];
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
      case 22:
        // Simulated active/locked bits follow the test mode, separate from controls.
        if ((word & 0x1000U) != 0U) word |= 0x0200U;
        if ((word & 0x2000U) != 0U) word |= 0x0800U;
        break;
      case 27: word = static_cast<uint16_t>(word | (uint16_t{bistCaptured} << 8U)); break;
      case 30:
        if ((word & 0x8000U) != 0U && !holdTdr) {
          if (tdrPolls++ == 0U) word &= 0xFFFDU;
          else words[address] = word = 0x0102;
        }
        break;
      default: break;
    }
    return word;
  }

  uint16_t read(uint8_t address) {
    if (address != 14 || (selector & 0xC000U) != 0x4000U) return readDirect(address);
    const auto devad = selector & 0x001FU;
    if (devad == 31) {
      if (offset < words.size()) return readDirect(static_cast<uint8_t>(offset));
      if (offset >= vendor.size()) return 0;
      if (!aGrade && offset == 0x0308U) return (vendor[0x0302] & 0x0040U) != 0U ? 1U : 0U;
      const auto word = vendor[offset];
      if (offset == 0x04A1U) vendor[offset] &= 0xFF00U; // RXFS selector survives status consumption.
      return word;
    }
    for (const auto& reg : extended) {
      if (reg.devad == devad && reg.offset == offset) return reg.value;
    }
    return 0;
  }

  void writeTarget(uint16_t devad, uint16_t target, uint16_t value) {
    if (traceWrites) {
      if (devad == 31 && target >= 0x04A5U && target <= 0x04A7U) {
        std::fprintf(stderr, "mdio-write %02x %04x redacted\n", unsigned(devad), unsigned(target));
      } else {
        std::fprintf(stderr, "mdio-write %02x %04x %04x\n", unsigned(devad), unsigned(target), unsigned(value));
      }
    }
    if (devad == 31 && target < words.size()) {
      const auto address = target;
      if (address == 0 && value == 0x8000U) {
        if (traceResets) std::fputs("reset 00 8000\n", stderr);
        words[0] = holdReset ? value : uint16_t{0x3100};
      } else if (address == 31 && (value == 0x8000U || value == 0x4000U)) {
        if (traceResets) std::fputs(value == 0x8000U ? "reset 1f 8000\n" : "reset 1f 4000\n", stderr);
        words[31] = holdReset ? value : uint16_t{0};
        if (value == 0x8000U) {
          words[0] = 0x3100;
          words[17] = 0;
          if (aGrade) resetAControls();
          words[22] = 0x0100;
          words[24] = aGrade ? 0x0400 : 0;
          words[27] = 0x007D;
          words[28] = 0x05EE;
          words[30] = 0x0102;
          vendor[0x0027] = 0;
          vendor[0x030E] = 0x8400;
          vendor[0x04D1] = 0x0080;
          for (auto& reg : extended) {
            if (reg.devad == 7U && reg.offset == 0x003CU) reg.value = 0;
          }
          if (regressHard) nowUs = 0;
        }
      } else if (address == 22) {
        words[address] = static_cast<uint16_t>((words[address] & 0x0100U) | (value & 0x705FU));
      } else if (address == 27) {
        words[address] = value & 0x00FFU;
        if ((value & 0x8000U) != 0U) { bistCaptured = bistErrors; bistErrors = 0; }
      } else if (address == 30) {
        // CDCR[1:0] are hardware status on A. Command writes cannot erase them.
        words[address] = aGrade ? static_cast<uint16_t>((value & 0xFFFCU) | (words[address] & 3U)) : value;
        if ((value & 0x8000U) != 0U) tdrPolls = 0;
      } else {
        // The AN restart bit clears independently of the modeled partner.
        words[address] = address == 0 ? static_cast<uint16_t>(value & 0xFDFFU) : value;
      }
      return;
    }
    if (devad == 31 && target < vendor.size()) {
      if (target == 0x04A0U) {
        // Writing one to acknowledgement bit 11 preserves the pending state.
        value = static_cast<uint16_t>((value & 0xF7FFU) | (value & vendor[target] & 0x0800U));
        if ((value & 0x0080U) != 0U) vendor[0x04A1] |= 0x1000U;
      }
      vendor[target] = value;
      return;
    }
    for (auto& reg : extended) {
      if (reg.devad == devad && reg.offset == target) { reg.value = value; return; }
    }
  }

  void write(uint8_t address, uint16_t value) {
    if (address == 13) { selector = value; return; }
    if (address != 14) { writeTarget(31, address, value); return; }
    const auto mode = selector & 0xC000U;
    if (mode == 0) { offset = value; return; }
    if (mode == 0x4000U) writeTarget(selector & 0x001FU, offset, value);
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
    config.variant = _options.variant;
    config.interface = _options.interface;
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
      if (!_options.campaignMode.empty()) {
        LinkSettings entry{};
        entry.advertisement.technologyMask = 15;
        entry.advertisement.asymmetricPause = true;
        entry.crossover.pairSelection = PairSelection::MDIX;
        entry.crossover.robust = true;
        entry.crossover.swapPair = entry.crossover.swapPolarity = true;
        status = _driver->startConfigure(entry, budget, token);
        for (unsigned i = 0; i < 128 && status.inProgress(); ++i) {
          status = _driver->pollOperation(token);
          if (status.inProgress()) _bus.nowUs += TICK_US;
        }
        if (!status.ok() || !_driver->takeResult(token, result).ok() || !result.status.ok()) return false;
      }
      LinkSnapshot link{};
      (void)_driver->readLinkStatus(link, {1000000, 10});
    }

    Identity identity{};
    identity.boot = _boot;
    identity.firmware = _options.identity;
    identity.target = "host";
    identity.backend = "sim-mdio";
    identity.console = "host-stdio";
    identity.clock = "simulated";
    identity.variant = config.variant;
    identity.interface = config.interface;
    identity.phyAddress = config.phyAddress;
    _owner.emplace(*_driver, maintenance, this, externalDisposition, attestFeature);
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
    else if (_paused && line.substr(0, 10) == "$fragment ") {
      // Deliver a partial CLI line. Test controls can now step the owner
      // before the remaining bytes and delimiter reach the same parser.
      for (char byte : line.substr(10)) {
        if (!stage(byte)) { _failed = true; break; }
      }
    }
    else if (line == "$raise-calibration") _bus.words[30] |= 0x4000U;
    else if (line == "$inject-bist-errors") _bus.bistErrors = 5;
    else if (line == "$inject-wake") {
      if ((_bus.vendor[0x04A0] & 0x0100U) != 0U) _bus.vendor[0x04A0] |= 0x0800U;
      _bus.vendor[0x04A1] |= 0x0001U;
      _bus.words[19] |= 0x0200U;
    }
    else if (line == "$latch-link-low") _bus.linkLatchedLow = true;
    else if (line == "$isolate") _bus.isolated = true;
    else if (line.substr(0, 15) == "$campaign-peer ") {
      const auto peer = campaignPartner(line.substr(15));
      if (!_bus.campaign || peer == 0xFFFFU) return false;
      _bus.peer = peer;
      _bus.peerReadyUs = 0;
      _bus.peerStartsDuringSample = false;
    } else if (line.substr(0, 23) == "$campaign-peer-delayed ") {
      const auto peer = campaignPartner(line.substr(23));
      if (!_bus.campaign || peer == 0xFFFFU) return false;
      _bus.peer = peer;
      _bus.peerReadyUs = _bus.nowUs + 6000U;
      _bus.peerStartsDuringSample = false;
    } else if (line.substr(0, 22) == "$campaign-peer-change ") {
      const auto peer = campaignPartner(line.substr(22));
      if (!_bus.campaign || peer == 0xFFFFU) return false;
      _bus.peer = peer;
      _bus.peerStartsDuringSample = true;
    }
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

  static bool attestFeature(void* context, Feature, uint32_t) noexcept {
    const auto& fixture = *static_cast<Fixture*>(context);
    return fixture._options.attestFeatures && !fixture._bus.isolated;
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
      "Usage: dp8382x_cli_fixture [--identity TOKEN] [--variant DP83826I|DP83826E|DP83826AI|DP83826AE] [--partner down|pending|100-full|10-half] "
      "[--fail-after N] [--reboot-after N] [--partial-record] [--deny-maintenance|--no-maintenance] "
      "[--hold-reset] [--regress-after-hard] [--trace-resets] [--trace-writes] [--mii] [--hold-tdr] "
      "[--attest-features] [--campaign basic|enhanced] [--campaign-peer PROFILE]\n";
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
