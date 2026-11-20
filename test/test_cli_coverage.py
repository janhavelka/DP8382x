"""Tie the CLI coverage rows to ledger owners and registered routes."""

import json
from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[1]
STAGE_44_ROUTES = {
    "connection identity", "connection probe", "connection discover",
    "configuration desired", "configuration observed", "configuration validate",
    "configuration apply", "configuration verify", "configuration invalidate",
    "registers read", "registers write", "registers modify", "registers dump",
    "registers bmcr",
}
STAGE_45_ROWS = {
    "access": {"test stress", "test stop", "test stats"},
    "reset": {"recovery reset", "recovery restart", "recovery reinitialize"},
    "initialization": {
        "connection initialize", "connection end", "owner start", "owner poll",
        "owner status", "owner result", "owner cancel", "owner abandon",
    },
    "link": {"link read", "link partner", "link cached", "link watch"},
    "counters": {"link collect", "cached events"},
    "straps": {"connection initialize", "recovery reinitialize"},
    "pcs_reset": {"recovery reset"},
    "hard_reset": {"recovery reset"},
    "restart": {"recovery restart"},
    "external_reset": {"recovery reinitialize"},
    "initialize_10base": {"connection initialize", "recovery reinitialize"},
    "observe_link": {"link read", "link partner", "link cached", "link watch"},
    "collect_event_counters": {"link collect", "cached events"},
}


def registered_tests():
    names = set()
    for file in ("CMakeLists.txt", "test/cli/CMakeLists.txt", "test/esp_idf/CMakeLists.txt"):
        cmake = (ROOT / file).read_text(encoding="utf-8")
        names.update(re.findall(r"add_test\(NAME ([a-z0-9_]+)\s", cmake))
        # The CTest suites used by this matrix are literal names or ITEMS loops.
        for items, body in re.findall(r"foreach\(suite IN ITEMS ([^)]*)\)(.*?)endforeach\(\)",
                                      cmake, re.S):
            for prefix in re.findall(r"add_test\(NAME ([a-z0-9_]+)\$\{suite\}", body):
                names.update(prefix + suite for suite in items.split())
    return names


class CliCoverageTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.ledger = json.loads((ROOT / "docs/registers/dp83826i.json").read_text(encoding="utf-8"))
        cls.features = {row["id"]: row for row in cls.ledger["features"]}
        cls.procedures = {row["id"]: row for row in cls.ledger["procedures"]}
        cls.matrix = (ROOT / "docs/feature-coverage.md").read_text(encoding="utf-8")

    def rows(self):
        rows = {}
        for line in self.matrix.splitlines():
            match = re.match(r"^\| `([a-z0-9_]+)`", line)
            if not match:
                continue
            identifier = match[1]
            self.assertNotIn(identifier, rows, f"Duplicate coverage row: {identifier}")
            cells = [cell.strip() for cell in line.strip("|").split("|")]
            self.assertEqual(len(cells), 9, identifier)
            self.assertTrue(all(cells), f"Incomplete coverage row: {identifier}")
            rows[identifier] = cells
        return rows

    def test_complete_feature_and_procedure_matrix(self):
        rows = self.rows()
        self.assertEqual(len(self.features), 35)
        self.assertEqual(len(self.procedures), 52)
        self.assertEqual(set(rows), self.features.keys() | self.procedures.keys())
        self.assertIn("DP83826I. BASIC and ENHANCED", self.matrix)
        for identifier, procedure in self.procedures.items():
            with self.subTest(procedure=identifier):
                cells = rows[identifier]
                self.assertEqual(cells[1].split(",", 1)[0], "/".join(procedure["modes"]))
                self.assertIn(f"Core {procedure['owner']['implementationStage']}.", cells[2])
                self.assertEqual(cells[3], procedure["owner"]["api"])
                self.assertEqual(cells[4], f"`{procedure['feature']}`")
                self.assertIn(identifier, self.features[procedure["feature"]]["procedures"])
        for identifier, feature in self.features.items():
            if feature["procedures"]:
                with self.subTest(feature=identifier):
                    self.assertCountEqual(rows[identifier][4].split(", "), feature["procedures"])

    def test_every_register_field_owner_has_a_route_or_disposition(self):
        rows = self.rows()
        fields = [field for register in self.ledger["registers"]
                  for field in register["fields"]]
        self.assertEqual(len(fields), 283)
        owners = {field["owner"] for field in fields}
        owners.update(action["feature"] for action in self.ledger["writeActions"])
        owners.update(address["feature"] for address in self.ledger["printedAddresses"])
        for owner in sorted(owners):
            with self.subTest(owner=owner):
                self.assertIn(owner, self.features)
                self.assertRegex(rows[owner][5], r"^(`[^`]+`|Pending (46|47)\b|N/A\.)")

    def test_listed_commands_have_dispatch_and_evidence(self):
        rows = self.rows()
        source = (ROOT / "examples/common/Dp8382xCli.cpp").read_text(encoding="utf-8")
        # Only the static table establishes that a name is accepted by dispatch.
        table = source.split("Processor::COMMANDS[] = {", 1)[1].split("\n};", 1)[0]
        registered = set(re.findall(r"\{\s*\"([^\"]+)\"\s*,", table))
        host_checks = registered_tests()
        documented = set()
        for identifier, cells in rows.items():
            with self.subTest(row=identifier):
                commands = set(re.findall(r"`([^`]+)`", cells[5]))
                self.assertLessEqual(commands, registered)
                if not commands:
                    self.assertRegex(cells[5], r"^(Pending (46|47)\b|N/A\.)")
                documented.update(commands)
                named_checks = set(re.findall(r"\b[a-z]+(?:_[a-z]+)+\b", cells[7]))
                self.assertTrue(named_checks, f"Missing host check: {identifier}")
                self.assertLessEqual(named_checks, host_checks)
                if commands & STAGE_44_ROUTES:
                    self.assertIn("`result`", cells[6])
                    self.assertIn("cli_fixture_transcript", cells[7])
                    self.assertIn("cli_processor_routes", cells[7])
                if commands & {"registers dump", "connection discover"}:
                    self.assertIn("`sample`", cells[6])
        self.assertLessEqual(STAGE_44_ROUTES, documented)
        for identifier in ("access", "negotiation", "crossover"):
            self.assertIn("native_cli_reconciliation", rows[identifier][7])

    def test_link_and_lifecycle_routes_keep_terminal_and_sample_evidence(self):
        rows = self.rows()
        for identifier, expected in STAGE_45_ROWS.items():
            with self.subTest(row=identifier):
                cells = rows[identifier]
                self.assertLessEqual(expected, set(re.findall(r"`([^`]+)`", cells[5])))
                self.assertIn("`result`", cells[6])
                self.assertIn("cli_fixture_transcript", cells[7])
                if expected & {"link watch", "test stress"}:
                    self.assertIn("`sample`", cells[6])
        for identifier in ("link", "counters", "observe_link", "collect_event_counters"):
            self.assertIn("native_cli_link_events", rows[identifier][7])
        self.assertIn("native_cli_lifecycle", rows["initialization"][7])


if __name__ == "__main__":
    unittest.main()
