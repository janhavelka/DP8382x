# Verification

## Current workspace

Run from the repository root using Python 3.10 or newer:

```sh
python tools/verify_references.py
python tools/verify_ledger.py
python -m unittest discover -s test -p "test_*.py" -v
git diff --check
```

The checker verifies every archived file's size and SHA-256, PDF signatures,
ZIP members and CRCs, derived-text hashes, archive inventory, and local Markdown
file links. It is offline and does not claim that TI has not published an update.
PDF parsing was also checked while collecting the initial archive.

The ledger validator checks the [part-specific schema](../docs/registers/README.md),
complete source inventory, masks, aliases, reset knowledge, citations and ownership.
`test_ledger.py` is registered through standard-library unittest discovery. It
tests valid/malformed temporary files and independently transcribed original-PDF
vectors, including every core field partition, access type and the pause matrix.
It also rejects missing core procedures, unproved MMD aliases and masks outside
their procedure feature's register coverage. Assertions
are unittest methods and remain active under Python optimization. Run both
commands as prompts 4–5 extend the ledger. These are research-tool tests, not
executing driver, package or hardware tests.

`test_diagnostic_ledger.py` extends registered discovery with original-PDF field,
access and reset literals for every prompt-4 detailed record, recipe actions and
legal encodings. Temporary malformed files prove rejection of dropped diagnostic
rows/procedures, invented/retargeted actions, changed recipe addresses and lost
required action ownership.

`test_remaining_ledger.py` covers every remaining detailed reset/access partition,
LED placement, odd-nibble polarity, pin legality, analog split fields, W0C neutral
writes, MAC/password ordering, nominal EEE timing and exact recipe actions.
Malformed temporary files reject incomplete source coverage, erased gates,
borrowed exceptions, fabricated EEE wire offsets and reclassified software scope.
All 74 registered tests are research-tool evidence only.

No driver, firmware build, CI workflow or physical hardware test exists at this stage.
The [80-prompt promptset](../docs/prompts/README.md) now defines those future
deliverables. Its [verification contract](../docs/prompts/VERIFICATION.md)
separates host, portable consumer, SDK link, package, remote CI and physical
evidence. See [planning validation](../docs/reports/promptset-validation.md) for
the checks actually performed on this documentation-only change.

## Planned implementation evidence

- Host tests: exact register transactions, identity/variant checks, advertisement
  masks, error propagation, bounded reset completion and partial-write failures.
- SDK integration: compile and link the example against a pinned ESP-IDF release.
- Hardware: boot and repeated reset, straps and clock, MDIO, 100/full link,
  reconnect/recovery, diagnostics and a separate 10 Mbit/s configuration.

Use datasheet behavior as the test oracle. Keep the evidence for each layer
separate; register mocks do not prove a linked firmware image or physical link.
