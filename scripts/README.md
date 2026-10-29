# Release version generation

`library.json` is the sole release-version source. It currently describes the
existing portable C++17 foundation and actual MIT license/repository; it does
not claim SDK, Arduino, PlatformIO or package publication support. The only
generated output is the committed `include/DP8382x/Version.h`. Neither CMake
nor ordinary consumers run this script.

```sh
python scripts/generate_version.py sync
python scripts/generate_version.py check
```

`sync` validates the whole JSON file, prepares the header bytes and replaces a
changed header through a temporary file in the same directory. Repeating it
without metadata changes leaves bytes and modification time unchanged. `check`
only reads and returns nonzero for stale, missing or invalid output. Both return
nonzero on input or I/O errors. Do not edit `Version.h` by hand.

The JSON root must contain exactly `name`, `version`, `description`, `license`
and `repository`, each a nonblank string. `name` is `DP8382x`; duplicate keys,
unknown/missing keys, malformed UTF-8/JSON and wrong types fail. Version syntax
is `MAJOR.MINOR.PATCH`: each ASCII decimal component is 0 through 65535 and has
no leading zero except `0`. Prerelease/build suffixes and `v` prefixes are not
accepted. Output is UTF-8 with LF line endings. The initial `0.1.0` labels the
portable foundation; prompts do not automatically increment it.

`Version.h` contains only release version constants. Firmware Git identity is
application build evidence, not core metadata. Later examples should refresh
their own build-directory identity file at each example build, writing only
when its content changes. If Git or a commit ID cannot be established, including
in an archive build, they must report Git identity as `unknown`. Ordinary core
configuration, compilation and runtime must remain independent of Git, Python,
Doxygen and sibling paths. Add future SDK/package version outputs to this one
generator only when those build paths exist; prepare all output before changing
any generated file.
