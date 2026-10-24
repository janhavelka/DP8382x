# Verification

## Current workspace

Run from the repository root using Python 3.10 or newer:

```sh
python tools/verify_references.py
git diff --check
```

The checker verifies every archived file's size and SHA-256, PDF signatures,
ZIP members and CRCs, derived-text hashes, archive inventory, and local Markdown
file links. It is offline and does not claim that TI has not published an update.
PDF parsing was also checked while collecting the initial archive.

No driver, firmware build, CI workflow or hardware test exists at this stage.

## Planned implementation evidence

- Host tests: exact register transactions, identity/variant checks, advertisement
  masks, error propagation, bounded reset completion and partial-write failures.
- SDK integration: compile and link the example against a pinned ESP-IDF release.
- Hardware: boot and repeated reset, straps and clock, MDIO, 100/full link,
  reconnect/recovery, diagnostics and a separate 10 Mbit/s configuration.

Use datasheet behavior as the test oracle. Keep the evidence for each layer
separate; register mocks do not prove a linked firmware image or physical link.
