"""Require both a successful example process and its expected stdout record."""

import re
import subprocess
import sys
import unittest


def check_output(expected, command):
    result = subprocess.run(command, capture_output=True, text=True, timeout=20)
    pattern = ".*?".join(r"(?<!\S)" + re.escape(field) + r"(?!\S)" for field in expected)
    if result.returncode or not expected or not re.search(pattern, result.stdout):
        raise SystemExit(f"{command!r} exited {result.returncode}, expected {expected!r} on stdout.\n"
                         f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}")


class ExampleOutputTests(unittest.TestCase):
    def test_requires_success_and_matching_stdout(self):
        expected = ["linkEvidence=CURRENT", "profile100full=yes"]
        record = " ".join(expected)
        check_output(expected, [sys.executable, "-c", f"print({record!r})"])
        for code in (f"print({record!r}); raise SystemExit(1)",
                     f"import sys; print({record!r}, file=sys.stderr)",
                     "print('linkEvidence=CURRENT\\nprofile100full=yes')",
                     "print('linkEvidence=CURRENT profile100full=yesterday')"):
            with self.subTest(code=code), self.assertRaises(SystemExit):
                check_output(expected, [sys.executable, "-c", code])


if __name__ == "__main__":
    if sys.argv[1:2] == ["--check"]:
        check_output(sys.argv[4:], sys.argv[2:4])
    else:
        unittest.main()
