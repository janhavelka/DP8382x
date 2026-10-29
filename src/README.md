# Implementation location

The prompt-9 foundation is header-only because its values and validators need
no compiled implementation. Device logic belongs here when it works; SDK-only
code belongs under `esp_idf/`. No driver or hardware-access stub is supplied yet.
See [scope and architecture](../docs/architecture.md).
