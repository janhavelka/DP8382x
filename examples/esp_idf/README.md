# Planned ESP-IDF examples

The first example should bring up the ESP32-P4 MAC with DP83826I, report PHY
identity and negotiated link state, and provide bounded register diagnostics.
It should accept board-specific pins, address, clock and advertisement policy.

There is no buildable example yet. Use the [board context](../../docs/esp32p4-board.md)
and [official SDK references](../../docs/reference/source-code.md) when implementing
it. Keep EtherCAT master integration in the application that owns that stack.

The [promptset](../../docs/prompts/README.md) separately plans an ordinary
Ethernet/IP example and an owner example demonstrating the 100/full EtherCAT
integration boundary, followed by a shared diagnostic CLI. Neither supplies an
EtherCAT master or servo-control implementation. Application power delivery is
outside the library and examples.

Both native examples will own MAC/controller creation, board settings, PHY
address, clock, scheduling and console transport. They will admit one executing
owner per PHY operation, serialize complete indirect MDIO transactions across
all bus participants and publish copied snapshots. They must retain partially
created resources and report cleanup failures until release is known. The
shared CLI will attach without starting hardware work; startup belongs to the
example. A separate plain C++ MCU consumer proves core portability, while the
native examples must be linked with the selected ESP-IDF SDK. Existing sibling
I2C examples do not establish an Ethernet backend or P4 link result.
