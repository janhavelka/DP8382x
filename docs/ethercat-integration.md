# Application ownership on ESP32-P4

The application requests autonegotiation advertising only 100BASE-TX full
duplex. The framework timer collects link observations. An application task
decides whether a session may use the port. Its admission decision runs under
the native PHY gate through `withAdmissionEvidence`, which supplies the link
sample, settings, health and MAC publication state from the same critical
section. Keep that callback short and synchronous. Log or notify application
code after the gate is released.

| Evidence | Application interpretation |
| --- | --- |
| Management health | Records MDIO outcomes. A successful cable-down observation is a successful management transaction. |
| Configuration | Check the requested and observed profile and VERIFIED trust. Advertising 100/full can succeed with the cable disconnected. |
| Link and MAC | Require a fresh, resolved 100/full observation and a matching running MAC publication. Withdraw permission on uncertainty, expiry or loss. |
| Session | Explicit admission assigns a generation. Recheck the generation and current evidence at the point of use. A later UP requires a new admission. |

A withdrawn session keeps responsibility for its outstanding buffers. Release
the old generation, establish bounded application-frame quiescence, then admit
one maintenance operation. Preserve its terminal result, including accepted
or possible hardware effects, before admitting separate restoration. Verified
recovery permits the application to request admission again. Application frame
quiescence and the SDK RX-task lifetime have different consequences. The
installed MAC, driver, PHY and callback context stay alive under the existing
`native_rx_quiescence` restriction.

The P4 board uses RMII with the PHY supplying REF_CLK. SNLA344C discusses an
EtherCAT slave controller. SNLA494 and SNLA495 address C2000 slave-controller
integration. Their ESC sequences and MII Fast Link Drop timing cannot establish
a detection deadline for this RMII application. Board power, straps, clock and
link transitions need an identified fixture. Raw MAC handoff belongs to prompt
42. Master scheduling, servo control and machine safety belong to the application.

See the [owner example and timing policy](../examples/esp_idf/owner/README.md)
and [native lifecycle and timing evidence](reference/library-sdk.md).
