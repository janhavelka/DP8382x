/// @file ConsoleBuffer.h
/// @brief Fixed console queue for human output and complete machine records.
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string_view>

namespace DP8382x::Cli {

/// @brief Queue output in order and keep machine records at the start of a line.
/// @pre A single application owner serializes all calls, including statistics.
/// @note The queue owns accepted bytes. Draining reports local acceptance only.
class ConsoleBuffer {
public:
  static constexpr std::size_t CAPACITY = 4096; ///< Total queued bytes.
  static constexpr std::size_t MAX_RECORD_BYTES = 1024; ///< Includes trailing CR LF.

  /// @brief Nonblocking writer accepting a prefix of the offered bytes.
  /// @param[in] context Borrowed application context, used only during the call.
  /// @param[in] bytes Borrowed span valid only until the callback returns.
  /// @param[in] size Maximum offered bytes.
  /// @return Bytes accepted from the beginning of the span, in the range 0..size.
  /// @pre Do not reenter the queue or retain the span.
  using WriteSome = std::size_t (*)(void* context, const char* bytes, std::size_t size);

  /// @brief Copy an entire human output chunk or count one dropped chunk.
  /// @param[in] chunk Human bytes, borrowed until return. ANSI styling counts
  /// as line content because the receiver checks the first byte of each line.
  /// @return True if queued. False leaves previously queued bytes unchanged.
  bool pushHuman(std::string_view chunk) noexcept {
    if (chunk.size() > CAPACITY - _size) {
      increment(_droppedHuman);
      return false;
    }
    append(chunk);
    if (!chunk.empty()) _lineOpen = chunk.back() != '\n';
    return true;
  }

  /// @brief Copy a complete ASCII record, preceded by CR LF if a line is open.
  /// @param[in] record Printable ASCII body and one trailing CR LF, at most
  /// MAX_RECORD_BYTES including its terminator. The serializer owns the prefix
  /// and fields. The leading separator, if needed, is outside the record.
  /// @return True if all bytes were queued. False changes neither bytes nor cursor.
  bool pushRecord(std::string_view record) noexcept {
    if (record.size() < 3 || record.size() > MAX_RECORD_BYTES ||
        record.substr(record.size() - 2) != "\r\n") return false;
    for (const char byte : record.substr(0, record.size() - 2)) {
      if (byte < ' ' || byte > '~') return false;
    }
    if (record.size() + (_lineOpen ? 2U : 0U) > CAPACITY - _size) return false;
    if (_lineOpen) append("\r\n");
    append(record);
    _lineOpen = false;
    return true;
  }

  /// @brief Offer at most one contiguous span, up to the byte budget.
  /// @param[in] context Borrowed writer context, used only during this call.
  /// @param[in] write Nonblocking writer. Null leaves the queue untouched.
  /// @param[in] budget Maximum bytes to offer. Zero leaves the queue untouched.
  /// @return Accepted bytes. A short write preserves the remaining bytes in order.
  /// @note An out-of-contract overcount is capped at the offered byte count.
  std::size_t drain(void* context, WriteSome write, std::size_t budget) noexcept {
    if (!write || _size == 0 || budget == 0) return 0;
    const std::size_t offered = std::min({_size, CAPACITY - _head, budget});
    const std::size_t accepted = std::min(write(context, _bytes + _head, offered), offered);
    if (accepted == 0) increment(_stalledWrites);
    _head = (_head + accepted) % CAPACITY;
    _size -= accepted;
    return accepted;
  }

  /// @brief Return pending bytes, including any queued line separator.
  /// @return Queue occupancy in bytes.
  std::size_t queuedBytes() const noexcept { return _size; }

  /// @brief Return whole human chunks dropped for lack of space.
  /// @return Cumulative count, saturating at the maximum uint32_t value.
  std::uint32_t droppedHuman() const noexcept { return _droppedHuman; }

  /// @brief Return writer calls that accepted zero of a nonempty span.
  /// @return Cumulative count, saturating at the maximum uint32_t value.
  std::uint32_t stalledWrites() const noexcept { return _stalledWrites; }

private:
  static void increment(std::uint32_t& counter) noexcept {
    if (counter != std::numeric_limits<std::uint32_t>::max()) ++counter;
  }

  void append(std::string_view bytes) noexcept {
    if (bytes.empty()) return;
    const std::size_t tail = (_head + _size) % CAPACITY;
    const std::size_t first = std::min(bytes.size(), CAPACITY - tail);
    std::memcpy(_bytes + tail, bytes.data(), first);
    if (first != bytes.size()) std::memcpy(_bytes, bytes.data() + first, bytes.size() - first);
    _size += bytes.size();
  }

  char _bytes[CAPACITY]{};
  std::size_t _head{0};
  std::size_t _size{0};
  std::uint32_t _droppedHuman{0};
  std::uint32_t _stalledWrites{0};
  bool _lineOpen{false}; // Logical tail, preserved when the writer empties the queue.
};
} // namespace DP8382x::Cli
