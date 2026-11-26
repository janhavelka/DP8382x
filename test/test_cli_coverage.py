"""Tie the CLI coverage rows to ledger owners and registered routes."""

from fnmatch import fnmatchcase
import json
from pathlib import Path
import re
import subprocess
import sys
import unittest


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from dp8382x_hil_campaign import all_cases

CTEST_INVOCATION = None
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
STAGE_46_TABLES = {
    "Dp8382xCliDiagnosticCommands.inc": "cli_diagnostic_transcript",
    "Dp8382xCliControlCommands.inc": "cli_control_transcript",
    "Dp8382xCliPinCommands.inc": "cli_pin_transcript",
}


def quoted(cell):
    return set(re.findall(r"`([^`]+)`", cell))


def named_checks(cell):
    # Notes describe evidence scope. Everything else must name a whole test.
    names = re.sub(r" \([^()]*\)", "", cell).replace(". Core: ", ", ")
    return set(names.split(", "))


class CliCoverageTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.ledger = json.loads((ROOT / "docs/registers/dp83826i.json").read_text(encoding="utf-8"))
        cls.features = {row["id"]: row for row in cls.ledger["features"]}
        cls.procedures = {row["id"]: row for row in cls.ledger["procedures"]}
        cls.matrix = (ROOT / "docs/feature-coverage.md").read_text(encoding="utf-8")

    def rows(self, matrix=None):
        rows = {}
        original = (self.matrix if matrix is None else matrix).split("## DP83826AI and DP83826AE", 1)[0]
        for line in original.splitlines():
            match = re.match(r"^\| `([a-z0-9_]+)`", line)
            if not match:
                continue
            identifier = match[1]
            self.assertNotIn(identifier, rows, f"Duplicate coverage row: {identifier}")
            cells = [cell.strip() for cell in line.strip("|").split("|")]
            self.assertEqual(len(cells), 9, identifier)
            self.assertTrue(all(cells), f"Incomplete coverage row: {identifier}")
            self.assertNotRegex(line, r"(?i)\b(pending [0-9]+|tbd|todo)\b")
            rows[identifier] = cells
        return rows

    def a_rows(self, matrix, heading):
        section = matrix.split("## DP83826AI and DP83826AE", 1)[1].split("### " + heading + "\n", 1)[1]
        rows = {}
        for line in section.split("\n### ", 1)[0].splitlines():
            if line.startswith("| `"):
                cells = [cell.strip() for cell in line.strip("|").split("|")]
                identifier = cells[0].strip("`")
                self.assertNotIn(identifier, rows, f"Duplicate A row: {identifier}")
                rows[identifier] = cells
        return rows

    def check_a_matrix(self, matrix):
        source = json.loads((ROOT / "docs/registers/dp83826a-procedures.json").read_text(encoding="utf-8"))
        fields = json.loads((ROOT / "docs/registers/dp83826a.json").read_text(encoding="utf-8"))
        procedures = self.a_rows(matrix, "Procedures")
        features = self.a_rows(matrix, "Features and qualification cases")
        registers = self.a_rows(matrix, "Register fields")
        recipes = self.a_rows(matrix, "Recipe access")
        original_routes = self.rows(matrix)
        for records, rows in ((source["procedures"], procedures), (source["features"], features),
                              (fields["registers"], registers), (fields["recipeRegisters"], recipes)):
            self.assertEqual(set(rows), {entry["id"] for entry in records})
        for entry in source["procedures"]:
            row = procedures[entry["id"]]
            self.assertEqual(row[1], f"`{entry['feature']}`")
            self.assertEqual(row[2], self.procedures[entry["id"]]["owner"]["api"].replace("; later", ", then"))
            identifier = entry["id"]
            commands = quoted(original_routes[identifier][5])
            if identifier in ("direct_access", "mmd_access"):
                commands -= {"registers write", "registers modify"}  # Raw LEDCR writes are fenced on A.
            self.assertEqual(quoted(row[3]), commands, identifier)
            self.assertTrue(quoted(row[4]))
        for entry in source["features"]:
            row = features[entry["id"]]
            self.assertEqual(quoted(row[1]), set(entry["procedures"]))
            self.assertEqual(quoted(row[2]), set(entry["gates"]))
            self.assertEqual(quoted(row[3]), quoted(original_routes[entry["id"]][8]), entry["id"])
        original = {f["id"]: f["owner"] for reg in self.ledger["registers"] for f in reg["fields"]}
        for reg in fields["registers"]:
            row = registers[reg["id"]]
            self.assertEqual(row[1:3], [f"{reg['space']} {reg['address']}", reg["table"]])
            groups = {}
            for field in reg["fields"]:
                owner = original.get(field["originalField"], "straps" if reg["id"] == "SOR1" else "led")
                groups.setdefault(owner, set()).add(f"{field['id']}={field['mask']}")
            if not groups:
                self.assertEqual(row[3], "No named fields")
                continue
            actual = {owner: quoted(words) for owner, words in
                      (group.split(" &mdash; ") for group in row[3].split("<br>"))}
            self.assertEqual(actual, groups, reg["id"])
        for reg in fields["recipeRegisters"]:
            row = recipes[reg["id"]]
            self.assertEqual(row[1], f"{reg['devad']}:{reg['address']}")
            self.assertEqual(row[2] == "Original only", reg["status"] == "ORIGINAL_ONLY")

        actions = self.a_rows(matrix, "Exceptional writes")
        owners = {row["id"]: row["feature"] for row in self.ledger["writeActions"]}
        expected = {}
        for action in source["writeActions"]:
            item = action["id"]
            if action.get("rows"):
                item += " @ " + ",".join(map(str, action["rows"]))
            expected.setdefault(owners.get(action["id"], "eee"), set()).add(item)
        self.assertEqual({owner: quoted(row[1]) for owner, row in actions.items()}, expected)

    def test_a_generation_coverage_and_negative_controls(self):
        self.check_a_matrix(self.matrix)
        lines = self.matrix.splitlines(keepends=True)
        led = next(line for line in lines if line.startswith("| `LEDCFG`"))
        for replacement in ("", led + led, led.replace("0x000F", "0x00F0")):
            with self.assertRaises(AssertionError):
                self.check_a_matrix(self.matrix.replace(led, replacement))
        original, a = self.matrix.split("## DP83826AI and DP83826AE", 1)
        for expected, wrong in (("`irq configure`", "`power configure`"), ("Case `bist-off`", "Case `eee`")):
            self.assertIn(expected, a)
            with self.assertRaises(AssertionError):
                self.check_a_matrix(original + "## DP83826AI and DP83826AE" + a.replace(expected, wrong, 1))

    def check_p25_matrix(self, matrix):
        source = json.loads((ROOT / "docs/registers/dp83825i.json").read_text(encoding="utf-8"))
        def table(heading):
            section = matrix.split("### DP83825I " + heading + "\n", 1)[1].split("\n### ", 1)[0]
            lines = [line.strip("|").split("|") for line in section.splitlines() if line.startswith("| `")]
            rows = {line[0].strip(" `"): [cell.strip() for cell in line[1:]] for line in lines}
            self.assertEqual(len(rows), len(lines))
            return rows
        routes = table("procedures")
        self.assertEqual(set(routes), {p["id"] for p in source["procedures"]})
        header = (ROOT / "include/DP8382x/DP8382x.h").read_text(encoding="utf-8")
        cli = ROOT / "examples/common"
        dispatch = "\n".join(path.read_text(encoding="utf-8") for path in
                             (cli / "Dp8382xCli.cpp", *(cli / name for name in STAGE_46_TABLES)))
        for api, command, test, scope in routes.values():
            self.assertTrue(scope)
            for symbol in api.split(" / "):
                self.assertRegex(header, r"\b" + re.escape(symbol) + r"\s*\(")
            verb, = quoted(command)
            self.assertIn(f'"{verb}"', dispatch)
            self.assertTrue(test.startswith("`p25_"), test)
        words = table("register fields")
        self.assertEqual(set(words), {r["offset"] for r in source["registers"]})
        for reg in source["registers"]:
            groups = {}
            for field in reg["fields"]:
                groups.setdefault(field["use"], []).append(field["bits"])
            owners = " / ".join(name + " (" + ", ".join(bits) + ")" for name, bits in groups.items())
            self.assertEqual(words[reg["offset"]], [reg["table"], owners])
            self.assertLessEqual(set(groups), set(routes) | {"reserved", "reserved_by_description"})
        exceptions = table("address exceptions")
        self.assertEqual(set(exceptions), {r["wireOffset"] for r in source["recipeOnly"]})
        for row in source["recipeOnly"]:
            owner, action = exceptions[row["wireOffset"]]
            self.assertEqual(quoted(owner), {row["procedure"]})
            self.assertIn(row["source"], action)

    def test_p25_field_and_procedure_join(self):
        self.check_p25_matrix(self.matrix)
        for old, new in (("`p25_functional_modes`", "`eee_configuration`"),
                         ("| `0333` | `vod`", "| `0333` | `quality`")):
            with self.assertRaises(AssertionError):
                self.check_p25_matrix(self.matrix.replace(old, new))

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

    def check_field_routes(self, rows):
        owners = {f["id"]: f["owner"] for reg in self.ledger["registers"] for f in reg["fields"]}
        self.assertEqual(len(owners), 283)
        for group in ("writeActions", "printedAddresses"):
            owners.update((item["id"], item["feature"]) for item in self.ledger[group])
        routes = {}
        for owner in self.features:
            cells = rows[owner]
            for group in cells[3].split("<br>")[1:]:
                label, selectors = group.split(" → ")
                self.assertRegex(label, r"^`[^`]+`(?:, `[^`]+`)*$")
                commands = quoted(label)
                self.assertLessEqual(commands, quoted(cells[5]), owner)
                for pattern in quoted(selectors):
                    matches = [name for name in owners if fnmatchcase(name, pattern)]
                    self.assertTrue(matches, pattern)
                    for name in matches:
                        self.assertEqual(owner, owners[name], name)
                        self.assertNotIn(name, routes)
                        routes[name] = commands
        self.assertEqual(routes.keys(), owners.keys())
        # Mixed words need distinct owners for observation, settings and commands.
        for field, command in {
            "RXFCFG1_levelAcknowledge": "wol acknowledge", "RXFCFG1_crcGate": "wol configure",
            "BICSR1_errorBytes": "diagnostics bist-read", "BICSR1_ipgLength": "diagnostics bist-start",
            "MISR1.link_changed_event": "irq service", "MISR1.link_changed_enable": "irq configure",
            "PHYCR.phy_address": "registers read", "PHYCR.pause_rx": "link partner",
            "PHYCR.mii_link": "registers read", "PHYSTS.link": "link collect",
            "ANER.page": "configuration apply", "10BTSCR.polarity": "controls quality",
        }.items():
            self.assertEqual(routes[field], {command}, field)
        for suffix in ("mdi_termination", "mdi_enable", "mdix_termination", "mdix_enable"):
            field = f"VOD_CFG3.{suffix}"
            self.assertEqual(routes[field], {"connection initialize", "controls analog"}, field)

    def test_every_register_field_owner_has_a_route_or_disposition(self):
        self.check_field_routes(self.rows())

    def check_physical_cases(self, rows):
        cases = all_cases().keys()
        for identifier, cells in rows.items():
            physical = cells[8]
            if identifier in ("absent", "outside_phy"):
                self.assertTrue(physical.startswith("N/A. "), identifier)
            elif identifier in ("next_page", "next_page_exchange"):
                self.assertIn("Manual Q10", physical)
                self.assertIn("partner", physical)
            else:
                self.assertIn("Case ", physical, identifier)
                selected = quoted(physical)
                self.assertTrue(selected, identifier)
                self.assertLessEqual(selected, cases, identifier)
                if identifier in self.procedures:
                    feature = self.procedures[identifier]["feature"]
                    self.assertLessEqual(selected, quoted(rows[feature][8]), identifier)
        for identifier, case in (("pcs_reset", "pcs"), ("hard_reset", "hard"),
                                 ("restart", "restart"), ("initialize_10base", "10base-te"),
                                 ("external_reset", "cold-boot"), ("power_deep", "power-deep")):
            self.assertIn(case, quoted(rows[identifier][8]), identifier)
        for identifier in ("counters", "collect_event_counters"):
            self.assertIn("Manual Q16", rows[identifier][8], identifier)
            self.assertIn("Manual Q17", rows[identifier][8], identifier)
        for identifier in ("crossover", "configure_forced"):
            self.assertIn("Manual Q11", rows[identifier][8], identifier)
        for owner, feature in self.features.items():
            if feature["procedures"]:
                covered = set().union(*(quoted(rows[p][8]) for p in feature["procedures"]))
                self.assertEqual(quoted(rows[owner][8]), covered, owner)

    def test_physical_cases(self):
        self.check_physical_cases(self.rows())

    def test_matrix_rejects_misdirected_or_incomplete_evidence(self):
        first = next(line for line in self.matrix.splitlines() if line.startswith("| `identity`"))
        with self.assertRaisesRegex(AssertionError, "Duplicate"):
            self.rows(first + "\n" + self.matrix)
        with self.assertRaises(AssertionError):
            self.rows(self.matrix.replace("`connection probe`", "Pending 47", 1))
        rows = self.rows()
        rows["wol"][3] = rows["wol"][3].replace("`wol acknowledge` →", "`wol configure` →")
        with self.assertRaisesRegex(AssertionError, "RXFCFG1_levelAcknowledge"):
            self.check_field_routes(rows)
        rows = self.rows()
        rows["initialization"][3] = rows["initialization"][3].replace(", `controls analog` →", " →")
        with self.assertRaisesRegex(AssertionError, "VOD_CFG3"):
            self.check_field_routes(rows)
        rows["hard_reset"][8] = "Case `pcs`"
        with self.assertRaisesRegex(AssertionError, "hard_reset"):
            self.check_physical_cases(rows)

    def check_registrations(self, names):
        for identifier, cells in self.rows().items():
            self.assertLessEqual(named_checks(cells[7]), names, identifier)

    def test_missing_registration_is_rejected(self):
        names = set().union(*(named_checks(cells[7]) for cells in self.rows().values()))
        with self.assertRaisesRegex(AssertionError, "identity_policy"):
            self.check_registrations(names - {"identity_policy"})

    def test_configured_ctest_registrations(self):
        if CTEST_INVOCATION is None:
            self.skipTest("run cli_matrix_registrations in the combined portable and pinned host build")
        ctest, build = CTEST_INVOCATION
        output = subprocess.check_output([ctest, "--test-dir", build, "--show-only=json-v1"],
                                         timeout=30, encoding="utf-8")
        tests = json.loads(output)["tests"]
        names = {test["name"] for test in tests if test.get("command") and not any(
            p["name"] == "DISABLED" and p["value"] for p in test["properties"])}
        self.check_registrations(names)
        exact = self.matrix.split("### DP83825I procedures\n", 1)[1].split("\n### ", 1)[0]
        for line in exact.splitlines():
            if line.startswith("| `"):
                registration, = quoted(line.split("|")[4])
                self.assertIn(registration, names)
        for row in self.a_rows(self.matrix, "Procedures").values():
            stem, = quoted(row[4])
            for grade in ("ai", "ae"):
                self.assertIn(f"{grade}_{stem}", names)

    def test_listed_commands_have_dispatch_and_evidence(self):
        rows = self.rows()
        source = (ROOT / "examples/common/Dp8382xCli.cpp").read_text(encoding="utf-8")
        # Only the static table establishes that a name is accepted by dispatch.
        table = source.split("Processor::COMMANDS[] = {", 1)[1].split("\n};", 1)[0]
        registered = set(re.findall(r"\{\s*\"([^\"]+)\"\s*,", table))
        feature_checks = {"diagnostics selftest": "cli_selftest_transcript"}
        for file, suite in STAGE_46_TABLES.items():
            self.assertIn(f'#include "{file}"', table)
            contents = (ROOT / "examples/common" / file).read_text(encoding="utf-8")
            commands = re.findall(r'^FEATURE\([A-Z0-9_]+, "([^"]+)"', contents, re.M)
            self.assertTrue(commands, file)
            for command in commands:
                self.assertNotIn(command, registered)
                registered.add(command)
                feature_checks[command] = suite
        documented = set()
        for identifier, cells in rows.items():
            with self.subTest(row=identifier):
                commands = set(re.findall(r"`([^`]+)`", cells[5]))
                self.assertLessEqual(commands, registered)
                if not commands:
                    self.assertEqual(identifier, "outside_phy")
                    self.assertTrue(cells[5].startswith("N/A. "))
                documented.update(commands)
                checks = named_checks(cells[7])
                for name in checks:
                    self.assertRegex(name, r"^[a-z][a-z0-9]*(?:_[a-z0-9]+)+$")
                if commands & STAGE_44_ROUTES:
                    self.assertIn("`result`", cells[6])
                    self.assertIn("cli_fixture_transcript", cells[7])
                    self.assertIn("cli_processor_routes", cells[7])
                if commands & {"registers dump", "connection discover"}:
                    self.assertIn("`sample`", cells[6])
                for command in commands & feature_checks.keys():
                    self.assertIn("`result`", cells[6])
                    self.assertIn(feature_checks[command], checks)
        self.assertLessEqual(STAGE_44_ROUTES, documented)
        self.assertLessEqual(feature_checks.keys(), documented)
        for identifier in ("access", "negotiation", "crossover"):
            self.assertIn("native_cli_reconciliation", rows[identifier][7])
        self.assertIn("native_cli_features", named_checks(rows["power"][7]))

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
    if sys.argv[1:2] == ["--ctest"]:
        CTEST_INVOCATION = sys.argv[2:4]
        del sys.argv[1:4]
    unittest.main()
