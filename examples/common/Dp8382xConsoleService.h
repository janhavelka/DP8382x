/// @file Dp8382xConsoleService.h
/// @brief Bounded transfers between the CLI owner, console I/O and task logs.
#pragma once

#include "Dp8382xCli.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string_view>

namespace DP8382x::Cli {

/// @brief Fixed ingress, CLI output and ordinary log queues for a native application.
/// @pre One I/O task offers input and drains output. One PHY owner task calls
/// service and supplies write as the processor's output sink. The object and
/// both tasks outlive all callbacks using it. Task logs may have many callers.
/// @note Neither ring waits. The short log try-lock never spans formatting,
/// driver calls or external console output. The external sink must be bounded.
class NativeConsole {
public:
  static constexpr std::size_t INPUT_CAPACITY = 512; ///< Queued terminal input bytes.
  static constexpr std::size_t OUTPUT_CAPACITY = 2048; ///< Bytes reserved for processor output.
  static constexpr std::size_t LOG_SLOTS = 8; ///< Queued log lines, plus one already draining.
  static constexpr std::size_t LOG_LINE_BYTES = 384; ///< Rendered log bytes including prefix and CR LF.
  static constexpr std::size_t DRAIN_BYTES = 256; ///< Maximum bytes offered in one sink call.

  /// @brief Counter snapshot. Counter values increment modulo 2^32.
  struct Stats {
    uint32_t queuedInput{0}; ///< Input bytes currently queued.
    uint32_t queuedOutput{0}; ///< Processor output bytes currently queued.
    uint32_t queuedLogs{0}; ///< Log lines awaiting service, excluding a line being drained.
    uint32_t droppedInput{0}; ///< Lost bytes reported by the input producer or dropped for capacity.
    uint32_t inputGaps{0}; ///< Input discontinuities, coalesced while a gap is pending.
    uint32_t droppedLogs{0}; ///< Dropped lines due to format, capacity or lock contention.
    uint32_t truncatedLogs{0}; ///< Overlong lines rejected whole, also counted in droppedLogs.
    uint32_t stalledWrites{0}; ///< External write attempts accepting no bytes.
    uint32_t outputBackpressure{0}; ///< Processor writes the output ring accepted only in part or refused.
  };

  /// @brief Offer input from the I/O task. Any unaccepted suffix is lost.
  /// @param[in] bytes Borrowed span, copied before return.
  /// @return Accepted prefix size. The caller must not retry the suffix.
  /// @note While an ordered gap is pending, discard newer bytes until the owner
  /// consumes the retained older bytes and marks the discontinuity.
  std::size_t offerInput(std::string_view bytes) noexcept {
    const auto accepted = _inputGap.load(std::memory_order_acquire) != 0U ? 0U : _input.push(bytes);
    noteInputLoss(bytes.size() - accepted);
    return accepted;
  }

  /// @brief Report a gap between inputs without discarding older queued bytes.
  /// @param[in] lostBytes Known loss size, or one for a loss of unknown size.
  /// @pre Called only by the I/O task, before offering later bytes.
  void noteInputLoss(std::size_t lostBytes = 1) noexcept {
    if (lostBytes == 0) return;
    _droppedInput.fetch_add(static_cast<uint32_t>(lostBytes), std::memory_order_relaxed);
    if (_inputGap.exchange(1U, std::memory_order_acq_rel) == 0U) {
      _inputGaps.fetch_add(1U, std::memory_order_relaxed);
    }
  }

  /// @brief Feed one bounded input span and service the processor once.
  /// @param[in,out] processor Sole CLI on the existing PHY owner task.
  /// @pre The processor is already attached to the application's callbacks.
  /// @note Retains a rejected input suffix. Feeds the gap ahead of any later
  /// bytes. Never calls the physical console sink or waits on output capacity.
  void service(Processor& processor) noexcept {
    const auto bytes = _input.peek(Processor::FEED_BYTES);
    if (!bytes.empty()) _input.consume(processor.feed(bytes));
    if (_inputGap.load(std::memory_order_acquire) != 0U && _input.peek(1).empty()) {
      processor.discardInput();
      _inputGap.store(0U, std::memory_order_release);
    }
    processor.tick();
  }

  /// @brief Copy a prefix of processor output into the reserved output ring.
  /// @param[in] bytes Borrowed bytes from Processor's write callback.
  /// @param[in] size Number of bytes in the span.
  /// @return Accepted prefix length. Processor owns everything else.
  /// @pre Only the PHY owner task calls this method. A nonzero size has valid bytes.
  std::size_t write(const char* bytes, std::size_t size) noexcept {
    if (size == 0) return 0;
    const auto accepted = _output.push({bytes, size});
    if (accepted != size) _outputBackpressure.fetch_add(1U, std::memory_order_relaxed);
    return accepted;
  }

  /// @brief Try to copy a complete ordinary line into the separate log queue.
  /// @param[in] line Borrowed text. One trailing LF or CR LF is optional.
  /// @return True if queued, false if too long, multiline, full or contended.
  /// @note Callable by multiple tasks. A human prefix prevents a log from
  /// masquerading as a machine record. Use the task logger, never an ISR hook.
  bool log(std::string_view line) noexcept {
    if (!line.empty() && line.back() == '\n') {
      line.remove_suffix(1);
      if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
    }
    if (line.size() > LOG_TEXT_BYTES) return dropLog(true);
    if (line.find_first_of("\r\n") != std::string_view::npos ||
        line.find('\0') != std::string_view::npos) return dropLog(false);
    if (_logLock.test_and_set(std::memory_order_acquire)) return dropLog(false);
    const auto depth = _logDepth.load(std::memory_order_relaxed);
    if (depth == LOG_SLOTS) {
      _logLock.clear(std::memory_order_release);
      return dropLog(false);
    }
    auto& queued = _logs[(_logHead + depth) % LOG_SLOTS];
    const auto textOffset = LOG_PREFIX.size();
    std::memcpy(queued.bytes.data(), LOG_PREFIX.data(), textOffset);
    if (!line.empty()) std::memcpy(queued.bytes.data() + textOffset, line.data(), line.size());
    queued.size = textOffset + line.size() + 2U;
    queued.bytes[queued.size - 2U] = '\r';
    queued.bytes[queued.size - 1U] = '\n';
    _logDepth.store(depth + 1U, std::memory_order_relaxed);
    _logLock.clear(std::memory_order_release);
    return true;
  }

  /// @brief Format one log line locally and try to queue it whole.
  /// @param[in] format Borrowed printf format for one line.
  /// @param[in] args Borrowed arguments, left intact for the caller.
  /// @return Formatted character count if accepted, zero if dropped.
  /// @note A line exceeding the bound is counted and discarded without a prefix.
  /// Pin a firmware producer to a format that makes one call for each line.
  int vprintf(const char* format, std::va_list args) noexcept {
    if (!format) { dropLog(false); return 0; }
    // Also allow a supplied CR LF at the end of a maximum-size log body.
    std::array<char, LOG_TEXT_BYTES + 3U> buffer{};
    std::va_list copied;
    va_copy(copied, args);
    const int textSize = std::vsnprintf(buffer.data(), buffer.size(), format, copied);
    va_end(copied);
    if (textSize < 0) { dropLog(false); return 0; }
    const auto size = static_cast<std::size_t>(textSize);
    if (size >= buffer.size()) { dropLog(true); return 0; }
    return log({buffer.data(), size}) ? textSize : 0;
  }

  /// @brief Offer at most one bounded span to a physical sink on the I/O task.
  /// @param[in] context Borrowed argument for the sink.
  /// @param[in] sink Nonblocking callback accepting a prefix. Null leaves the queues untouched.
  /// @return Bytes accepted by the sink. A short or zero write retains the suffix.
  /// @note Processor bytes lead. Once a log starts it completes before other
  /// output. A pending record prevents logs even between its producer chunks.
  std::size_t drain(void* context, ConsoleBuffer::WriteSome sink) noexcept {
    if (!sink) return 0;
    if (!_logStarted) {
      const auto bytes = _output.peek(DRAIN_BYTES);
      if (!bytes.empty()) {
        const auto accepted = writeSome(context, sink, bytes);
        for (const char byte : bytes.substr(0, accepted)) {
          if (byte == '\n') _line = Line::CLOSED;
          else if (_line == Line::CLOSED) _line = byte == '@' ? Line::RECORD : Line::HUMAN;
        }
        _output.consume(accepted);
        return accepted;
      }
      if (_line == Line::RECORD) return 0;
      if (_activeSize == 0 && !loadLog()) return 0;
      // Processor may leave a human prompt open while idle. Its own record
      // separator remains in its stream, so subsequent records stay intact.
      _activeOffset = _line == Line::CLOSED ? 2U : 0U;
    }
    const auto accepted = writeSome(context, sink, {_activeLog.data() + _activeOffset,
        std::min(DRAIN_BYTES, _activeSize - _activeOffset)});
    if (accepted != 0) {
      _activeOffset += accepted;
      _logStarted = _activeOffset != _activeSize;
      if (!_logStarted) { _activeSize = 0; _line = Line::CLOSED; }
    }
    return accepted;
  }

  /// @brief Copy independent queue and counter values without accessing the PHY.
  /// @return Approximate occupancies during concurrent calls, with no reset of totals.
  Stats stats() const noexcept {
    return {_input.occupancy(), _output.occupancy(), _logDepth.load(std::memory_order_relaxed),
            _droppedInput.load(std::memory_order_relaxed), _inputGaps.load(std::memory_order_relaxed),
            _droppedLogs.load(std::memory_order_relaxed), _truncatedLogs.load(std::memory_order_relaxed),
            _stalledWrites.load(std::memory_order_relaxed), _outputBackpressure.load(std::memory_order_relaxed)};
  }

private:
  static_assert(std::atomic<uint32_t>::is_always_lock_free, "Console queues need lock-free 32-bit atomics");
  inline static constexpr std::string_view LOG_PREFIX = "[log] ";
  static constexpr std::size_t LOG_TEXT_BYTES = LOG_LINE_BYTES - LOG_PREFIX.size() - 2U;

  template<uint32_t Capacity>
  class ByteRing {
    static_assert(Capacity != 0 && (Capacity & (Capacity - 1U)) == 0);

  public:
    std::size_t push(std::string_view bytes) noexcept {
      const auto head = _head.load(std::memory_order_relaxed);
      const auto tail = _tail.load(std::memory_order_acquire);
      const auto count = std::min<std::size_t>(Capacity - (head - tail), bytes.size());
      const auto index = head % Capacity;
      const auto first = std::min<std::size_t>(Capacity - index, count);
      if (first != 0) std::memcpy(_bytes.data() + index, bytes.data(), first);
      if (count != first) std::memcpy(_bytes.data(), bytes.data() + first, count - first);
      _head.store(head + static_cast<uint32_t>(count), std::memory_order_release);
      return count;
    }

    std::string_view peek(std::size_t limit) const noexcept {
      const auto tail = _tail.load(std::memory_order_relaxed);
      const auto head = _head.load(std::memory_order_acquire);
      const auto index = tail % Capacity;
      return {_bytes.data() + index, std::min<std::size_t>({head - tail, Capacity - index, limit})};
    }

    void consume(std::size_t size) noexcept {
      _tail.fetch_add(static_cast<uint32_t>(size), std::memory_order_release);
    }

    uint32_t occupancy() const noexcept {
      const auto tail = _tail.load(std::memory_order_acquire);
      const auto count = _head.load(std::memory_order_acquire) - tail;
      return std::min(Capacity, count);
    }

  private:
    std::array<char, Capacity> _bytes{};
    std::atomic<uint32_t> _head{0}, _tail{0};
  };

  struct LogLine {
    std::array<char, LOG_LINE_BYTES> bytes{};
    std::size_t size{0};
  };
  enum class Line { CLOSED, HUMAN, RECORD };

  bool dropLog(bool truncated) noexcept {
    _droppedLogs.fetch_add(1U, std::memory_order_relaxed);
    if (truncated) _truncatedLogs.fetch_add(1U, std::memory_order_relaxed);
    return false;
  }

  bool loadLog() noexcept {
    if (_logLock.test_and_set(std::memory_order_acquire)) return false;
    const auto depth = _logDepth.load(std::memory_order_relaxed);
    if (depth != 0) {
      const auto& next = _logs[_logHead];
      std::memcpy(_activeLog.data() + 2U, next.bytes.data(), next.size);
      _activeSize = next.size + 2U;
      _logHead = (_logHead + 1U) % LOG_SLOTS;
      _logDepth.store(depth - 1U, std::memory_order_relaxed);
    }
    _logLock.clear(std::memory_order_release);
    return depth != 0;
  }

  std::size_t writeSome(void* context, ConsoleBuffer::WriteSome sink, std::string_view bytes) noexcept {
    const auto accepted = std::min(sink(context, bytes.data(), bytes.size()), bytes.size());
    if (accepted == 0) _stalledWrites.fetch_add(1U, std::memory_order_relaxed);
    return accepted;
  }

  ByteRing<INPUT_CAPACITY> _input;
  ByteRing<OUTPUT_CAPACITY> _output;
  std::atomic<uint32_t> _inputGap{0};
  std::atomic<uint32_t> _droppedInput{0}, _inputGaps{0}, _droppedLogs{0}, _truncatedLogs{0};
  std::atomic<uint32_t> _stalledWrites{0}, _outputBackpressure{0};
  std::array<LogLine, LOG_SLOTS> _logs{};
  std::atomic_flag _logLock = ATOMIC_FLAG_INIT;
  std::atomic<uint32_t> _logDepth{0};
  std::size_t _logHead{0};
  // Only the I/O task uses the active line and cursor. Leading CR LF is
  // reserved to close an open human line, if needed.
  std::array<char, LOG_LINE_BYTES + 2U> _activeLog{'\r', '\n'};
  std::size_t _activeSize{0}, _activeOffset{0};
  bool _logStarted{false};
  Line _line{Line::CLOSED};
};

} // namespace DP8382x::Cli
