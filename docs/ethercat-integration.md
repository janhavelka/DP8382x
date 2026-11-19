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
link transitions need an identified fixture. Master scheduling, servo control
and machine safety belong to the application.

## Frame ownership on the pinned P4 driver

| Boundary | Ownership and evidence |
| --- | --- |
| `esp_err_t stack_input(esp_eth_handle_t hdl, uint8_t* buffer, uint32_t length, void* priv)` | Registered once with `esp_eth_update_input_path` while stopped. The P4 RX task uses the extended mediator entry, which forwards to this callback. The raw owner copies at most 1,518 bytes into application storage and frees the original on every return, including drops and errors. No netif or glue is attached to this port. |
| `esp_err_t esp_eth_transmit(esp_eth_handle_t hdl, void* buf, size_t length)` | One application task lends a buffer until the call returns. The P4 copies into DMA buffers. ESP_OK means queued. ESP_ERR_NO_MEM records backpressure and drops the request. ESP_ERR_INVALID_STATE reports the SDK's cached link-down and withdraws the session. The caller retains its bytes on every outcome. |
| Release and retirement | Close session admission, release the generation and take the RX queue guard once before discarding session slots for maintenance. A late callback can still drop and free. Queued-to-DMA frames have no wire-completion receipt here. `Phy::retire` retains the installed driver, MAC, PHY and static callback context under `native_rx_quiescence`. |

The application allocates four slots per direction and services at most one
slot from each per turn. A dequeued TX carries its original generation until
the synchronous SDK call. The final check validates its length, handle,
generation and sample age. A regressing owner clock withdraws admission even
if the regression falls between frame submission and the next service turn.
The service-gap budget is measured between owner checks.

After an SDK cached DOWN, the PHY can continue to report the same UP tuple
while the driver suppresses duplicate publications. Release the withdrawn
generation, quiesce the application queues and request verified maintenance.
Only a successful framework restart clears `frameworkRestartRequired`.
A fresh post-maintenance sample then permits explicit readmission.

An RX callback entering after readmission can report
only its current delivery generation. The example counts EtherTypes and never
automatically replies. Wire capture on a known peer would establish actual
delivery there. Cycle period, latency, loss under load and master behavior
require separate measurements. The [example build and commands](../examples/esp_idf/owner/README.md)
describe both opt-ins for generating a single test frame.

See the [owner example and timing policy](../examples/esp_idf/owner/README.md)
and [native lifecycle and timing evidence](reference/library-sdk.md).
