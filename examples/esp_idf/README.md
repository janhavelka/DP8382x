# Planned ESP-IDF example

The first example should bring up the ESP32-P4 MAC with DP83826I, report PHY
identity and negotiated link state, and provide bounded register diagnostics.
It should accept board-specific pins, address, clock and advertisement policy.

There is no buildable example yet. Use the [board context](../../docs/esp32p4-board.md)
and [official SDK references](../../docs/reference/source-code.md) when implementing
it. Keep EtherCAT master integration in the application that owns that stack.
