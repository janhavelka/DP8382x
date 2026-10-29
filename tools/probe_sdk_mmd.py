"""Compile/run a finite research probe against an exact externally supplied SDK header.

No download, SDK mutation or production-driver test occurs. Temporary compiler
outputs are isolated; a mismatched source is rejected before compilation.
"""

from __future__ import annotations

import argparse
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile


HEADER_BLOB = "e9091f63a39ac1861bb043b5f25373cbb0d749f0"
ROOT = Path(__file__).resolve().parents[1]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("sdk_header", type=Path,
                        help="v6.1 eth_phy_802_3_regs.h at the documented commit")
    parser.add_argument("--compiler", default="gcc", help="GCC-compatible C compiler")
    args = parser.parse_args()
    try:
        payload = args.sdk_header.read_bytes()
        blob = hashlib.sha1(b"blob " + str(len(payload)).encode("ascii") + b"\0" + payload).hexdigest()
        if blob != HEADER_BLOB:
            print(f"FAIL: SDK header blob {blob}; expected {HEADER_BLOB}", file=sys.stderr)
            return 1
        with tempfile.TemporaryDirectory(prefix="dp8382x-mmd-probe-") as directory:
            temporary = Path(directory)
            (temporary / "eth_phy_802_3_regs.h").write_bytes(payload)
            executable = temporary / ("probe.exe" if os.name == "nt" else "probe")
            subprocess.run([args.compiler, "-std=c11", "-Wall", "-Wextra", "-Werror",
                            "-O2", "-DNDEBUG", "-I", str(temporary),
                            str(ROOT / "test/sdk_mmd_poison.c"), "-o", str(executable)],
                           check=True, timeout=30)
            subprocess.run([str(executable)], check=True, timeout=10)
        print(f"PASS: exact SDK header blob {blob}")
        return 0
    except (OSError, subprocess.SubprocessError) as exc:
        print(f"FAIL: probe could not complete: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
