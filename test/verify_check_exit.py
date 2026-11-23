"""Verify the CHECK runner distinguishes a failed assertion from a usage error."""

import argparse
import subprocess
import sys


def verify(binary, argument, code, summary, diagnostic):
    result = subprocess.run([binary, argument], capture_output=True, text=True, timeout=15)
    if (result.returncode != code or result.stdout.strip() != summary
            or diagnostic not in result.stderr):
        sys.exit(f"{argument} exited {result.returncode}, stdout={result.stdout!r}, "
                 f"stderr={result.stderr!r}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary")
    parser.add_argument("--empty-suite", action="store_true")
    parser.add_argument("--incomplete-log", action="store_true")
    args = parser.parse_args()
    verify(args.binary, "--verify-failure-path", 1, "1 checks; 1 failures",
           "Intentional Release failure probe")
    verify(args.binary, "--unknown-suite", 2, "", "")
    if args.empty_suite:
        verify(args.binary, "--verify-empty-suite", 1, "0 checks; 0 failures",
               "Suite --verify-empty-suite executed zero checks")
    if args.incomplete_log:
        verify(args.binary, "--verify-incomplete-log", 1, "1 checks; 1 failures",
               "end != std::string_view::npos")
