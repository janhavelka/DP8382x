"""Offline, part-specific structural and coverage checks for the research ledger.

This validates research records, not a driver or physical PHY. Numeric silicon
oracles belong in test/test_ledger.py and docs/register-vectors.md.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import sys


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_LEDGER = ROOT / "docs/registers/dp83826i.json"
ACCESS = {"R", "RH", "RC", "RCH", "RW", "RHW", "W1S", "W0C", "W1C"}
EVIDENCE = {"COMPLETE", "PARTIAL", "PENDING", "NOT_RUN", "NOT_APPLICABLE"}
CORE = {
    "BMCR", "BMSR", "PHYIDR1", "PHYIDR2", "ANAR", "ALNPAR", "ANER",
    "ANNPTR", "ANLNPTR", "CR1", "CR2", "CR3", "REGCR", "ADDAR", "PHYSTS",
    "PHYCR", "PHYRCR", "VOD_CFG3", "SOR1", "SOR2",
}
CORE_PROCEDURES = {
    "identify": "identity", "direct_access": "access", "mmd_access": "access",
    "pcs_reset": "reset", "hard_reset": "reset", "restart": "reset",
    "external_reset": "reset", "initialize_10base": "initialization",
    "configure_an": "negotiation", "configure_forced": "crossover",
    "next_page_exchange": "next_page", "observe_link": "link",
}
DIAGNOSTIC_REGISTERS = {
    "FLDS", "PHYSCR", "MISR1", "MISR2", "FCSCR", "RECR", "BISCR", "RCSR",
    "10BTSCR", "BICSR1", "BICSR2", "CDCR", "FLD_CFG1", "FLD_CFG2", "CDSCR",
    "CDSCR2", "CDSCR3", "TDR_175", "TDR_176", "CDSCR4", "TDR_178",
    "CDLRR1", "CDLRR2", "CDLRR3", "CDLRR4", "CDLRR5", "CDLAR1", "CDLAR2",
    "CDLAR3", "CDLAR4", "CDLAR5", "CDLAR6", "MSE_Val", "RXFS",
}
DIAGNOSTIC_RECIPES = {"RECIPE_0123", "LOOP_0122", "LOOP_0130"}
DIAGNOSTIC_PROCEDURES = {
    **dict.fromkeys(("loopback_near", "loopback_digital", "loopback_reverse",
                     "loopback_external"), "loopback"),
    "bist_run": "bist", "bist_capture": "bist",
    **dict.fromkeys(("cable_configure", "cable_manual", "cable_autorun", "cable_capture"), "cable"),
    "configure_fast_drop": "fast_drop",
    "interrupt_configuration": "interrupts", "interrupt_test": "interrupts",
    "collect_event_counters": "counters", "signal_quality_sample": "quality",
    "calibration_recovery": "calibration",
}
# Exact exceptions from SNLS647I 8.3.14.4 and table 8-45, not general write access.
# register, mask, value, command-neutral value (None for whole-word recipe), owner
WRITE_ACTIONS = {
    "BICSR1_freezeClear": ("BICSR1", 0x8000, 0x8000, 0, "bist"),
    "LOOP_0122_digital": ("LOOP_0122", 0xFFFF, 0x2000, None, "loopback"),
    "RECIPE_0123_digital": ("RECIPE_0123", 0xFFFF, 0x2000, None, "loopback"),
    "LOOP_0130_digital": ("LOOP_0130", 0xFFFF, 0x47FF, None, "loopback"),
}
PROCEDURE_ACTIONS = {
    "bist_capture": {"BICSR1_freezeClear"},
    "loopback_digital": {"LOOP_0122_digital", "RECIPE_0123_digital", "LOOP_0130_digital"},
}
# Independent transcription of SNLS647I table 8-17 (79 primary addresses).
C22_INVENTORY = {
    0x00: "BMCR", 0x01: "BMSR", 0x02: "PHYIDR1", 0x03: "PHYIDR2",
    0x04: "ANAR", 0x05: "ALNPAR", 0x06: "ANER", 0x07: "ANNPTR",
    0x08: "ANLNPTR", 0x09: "CR1", 0x0A: "CR2", 0x0B: "CR3",
    0x0D: "REGCR", 0x0E: "ADDAR", 0x0F: "FLDS", 0x10: "PHYSTS",
    0x11: "PHYSCR", 0x12: "MISR1", 0x13: "MISR2", 0x14: "FCSCR",
    0x15: "RECR", 0x16: "BISCR", 0x17: "RCSR", 0x18: "LEDCR",
    0x19: "PHYCR", 0x1A: "10BTSCR", 0x1B: "BICSR1", 0x1C: "BICSR2",
    0x1E: "CDCR", 0x1F: "PHYRCR",
}
MMD_INVENTORY = {
    0x0025: "MLEDCR", 0x0027: "COMPT", 0x002A: "10M_CFG",
    0x0117: "FLD_CFG1", 0x0131: "FLD_CFG2", 0x0170: "CDSCR",
    0x0171: "CDSCR2", 0x0173: "CDSCR3", 0x0175: "TDR_175",
    0x0176: "TDR_176", 0x0177: "CDSCR4", 0x0178: "TDR_178",
    0x0180: "CDLRR1", 0x0181: "CDLRR2", 0x0182: "CDLRR3",
    0x0183: "CDLRR4", 0x0184: "CDLRR5", 0x0185: "CDLAR1",
    0x0186: "CDLAR2", 0x0187: "CDLAR3", 0x0188: "CDLAR4",
    0x0189: "CDLAR5", 0x018A: "CDLAR6", 0x0218: "MSE_Val",
    0x0302: "IO_CFG1", 0x0303: "LED0_GPIO_CFG", 0x0304: "LED1_GPIO_CFG",
    0x0305: "LED2_GPIO_CFG", 0x0306: "LED3_GPIO_CFG",
    0x0308: "CLK_OUT_LED_STATUS", 0x030B: "VOD_CFG1", 0x030C: "VOD_CFG2",
    0x030E: "VOD_CFG3", 0x0404: "ANA_LD_PROG_SL", 0x040D: "ANA_RX10BT_CTRL",
    0x0456: "GENCFG", 0x0460: "LEDCFG", 0x0461: "IOCTRL", 0x0467: "SOR1",
    0x0468: "SOR2", 0x0469: "LEDCFG2", 0x04A0: "RXFCFG1", 0x04A1: "RXFS",
    0x04A2: "RXFPMD1", 0x04A3: "RXFPMD2", 0x04A4: "RXFPMD3",
    0x04A5: "RXFSOP1", 0x04A6: "RXFSOP2", 0x04A7: "RXFSOP3",
}
RECIPE_OFFSETS = {
    0x04D1, 0x04D3, 0x04DF, 0x033E, 0x033F, 0x0123, 0x031B, 0x0466,
    0x04CF, 0x0416, 0x04F5, 0x04E0, 0x031F, 0x04F4, 0x0428, 0x0122, 0x0130,
}
PRINTED_ADDRESSES = {(3, 0x1001), (3, 0x1014), (3, 0x1016),
                     (7, 0x203C), (7, 0x203D)}
FEATURE_STAGES = {
    **dict.fromkeys(("identity", "access", "reset", "initialization", "negotiation",
                     "next_page", "crossover", "link", "straps", "outside_phy", "absent"), 3),
    **dict.fromkeys(("loopback", "bist", "cable", "interrupts", "counters", "quality",
                     "fast_drop", "calibration"), 4),
    **dict.fromkeys(("eee", "lpi", "wol", "power", "repeater", "clock", "mii", "rmii",
                     "tx100", "tx10", "link_recovery", "led", "pin_mux", "compliance",
                     "analog", "compatibility"), 5),
}


class LedgerError(ValueError):
    """An actionable structural failure, including its record location."""


def require(condition: bool, location: str, message: str) -> None:
    if not condition:
        raise LedgerError(f"{location}: {message}")


def text(value: object, location: str) -> str:
    require(isinstance(value, str) and bool(value.strip()), location,
            "expected nonempty text")
    return value


def integer(value: object, location: str, low: int, high: int) -> int:
    require(type(value) is int and low <= value <= high, location,
            f"expected integer in {low}..{high}")
    return value


def hex_value(value: object, location: str, maximum: int = 0xFFFF) -> int:
    require(isinstance(value, str) and re.fullmatch(r"0x[0-9A-F]+", value)
            is not None, location, "expected uppercase hexadecimal string")
    number = int(value, 16)
    require(number <= maximum, location, f"value exceeds 0x{maximum:X}")
    return number


def sequence(value: object, location: str, nonempty: bool = False) -> list:
    require(isinstance(value, list), location, "expected array")
    require(not nonempty or bool(value), location, "array must not be empty")
    return value


def record(value: object, location: str, keys: str) -> dict:
    require(isinstance(value, dict), location, "expected object")
    missing = set(keys.split()) - value.keys()
    require(not missing, location, f"missing keys: {', '.join(sorted(missing))}")
    return value


def contract(value: object, location: str, allow_empty: bool = False) -> None:
    if isinstance(value, list):
        sequence(value, location, nonempty=not allow_empty)
        for index, item in enumerate(value):
            text(item, f"{location}[{index}]")
    else:
        text(value, location)


def address(value: dict, location: str) -> tuple[str, int | None, int]:
    record(value, location, "space devad address")
    require(isinstance(value["space"], str) and value["space"] in {"C22", "MMD"},
            location, "unknown address space")
    if value["space"] == "C22":
        require(value["devad"] is None, location, "C22 DEVAD must be null")
        return "C22", None, hex_value(value["address"], location + ".address", 31)
    return ("MMD", hex_value(value["devad"], location + ".devad", 31),
            hex_value(value["address"], location + ".address"))


def validate(data: object, root: Path = ROOT) -> None:
    """Validate a parsed ledger; raise LedgerError on the first useful failure."""
    data = record(data, "ledger", "schemaVersion variant specification sources identity "
                  "timings registers procedures features gates printedAddresses pauseResolution writeActions")
    require(type(data["schemaVersion"]) is int and data["schemaVersion"] == 1,
            "schemaVersion", "only version 1 is supported")
    require(data["variant"] == "DP83826I", "variant", "expected DP83826I")
    require(data["specification"] == "SNLS647I", "specification", "expected SNLS647I")
    ids: set[str] = set()

    def identify(item: dict, location: str) -> str:
        identity = text(item.get("id"), location + ".id")
        require(identity not in ids, location, f"duplicate semantic ID {identity}")
        ids.add(identity)
        return identity

    def entries(key: str, required: str) -> dict[str, dict]:
        result = {}
        for index, item in enumerate(sequence(data[key], key, nonempty=True)):
            location = f"{key}[{index}]"
            record(item, location, "id " + required)
            result[identify(item, location)] = item
        return result

    sources = entries("sources", "path revision authority license")
    for sid, item in sources.items():
        for key in ("path", "revision", "authority", "license"):
            text(item[key], f"{sid}.{key}")
        source_path = (root / item["path"]).resolve()
        require(source_path.is_relative_to(root.resolve()) and source_path.is_file(),
                sid, "source path must name an existing file inside the repository")

    def citations(value: object, location: str) -> None:
        for index, item in enumerate(sequence(value, location, nonempty=True)):
            at = f"{location}[{index}]"
            record(item, at, "source location")
            text(item["source"], at + ".source")
            require(item["source"] in sources, at, "unknown source reference")
            text(item["location"], at + ".location")

    def refs(value: object, choices: dict, location: str,
             nonempty: bool = False) -> list[str]:
        result = sequence(value, location, nonempty)
        seen = set()
        for reference in result:
            text(reference, location)
            require(reference in choices, location, f"unknown reference {reference}")
            require(reference not in seen, location, f"duplicate reference {reference}")
            seen.add(reference)
        return result

    def modes(value: object, location: str) -> None:
        values = sequence(value, location, nonempty=True)
        require(all(isinstance(v, str) and v in {"BASIC", "ENHANCED"} for v in values),
                location, "unknown operating mode")
        require(len(set(values)) == len(values), location, "duplicate mode")

    def owner(value: object, location: str) -> None:
        record(value, location, "api cli test implementationStage")
        for key in ("api", "cli", "test"):
            contract(value[key], f"{location}.{key}")
        integer(value["implementationStage"], location + ".implementationStage", 1, 80)

    gates = entries("gates", "conflict decision nextEvidence ownerStage sources")
    for gid, item in gates.items():
        for key in ("conflict", "decision", "nextEvidence"):
            text(item[key], f"{gid}.{key}")
        integer(item["ownerStage"], gid + ".ownerStage", 1, 80)
        citations(item["sources"], gid + ".sources")

    registers = entries("registers", "name modes space devad address width aliases "
                        "resetValue resetKnownMask resetNotes reservedMask access readEffects "
                        "sources researchStage status fields gates")
    features = entries("features", "name researchStage implementationStages registers procedures "
                       "owner scope evidence gates notes")
    for fid, item in features.items():
        sequence(item["registers"], fid + ".registers")
    procedures = entries("procedures", "feature modes sources ownedMasks prerequisites steps "
                         "waits timeout partialEffects cleanup restoration gates owner")
    actions = entries("writeActions", "register mask value neutral effect prerequisites sources gates feature")
    occupied: dict[tuple, str] = {}
    primary: dict[tuple, str] = {}
    names = set()
    for rid, item in registers.items():
        text(item["name"], rid + ".name")
        require(item["name"] not in names, rid, "duplicate register name")
        names.add(item["name"])
        modes(item["modes"], rid + ".modes")
        require(type(item["width"]) is int and item["width"] == 16, rid,
                "register width must be 16")
        integer(item["researchStage"], rid + ".researchStage", 3, 5)
        require(isinstance(item["status"], str) and item["status"] in {"COMPLETE", "PENDING", "RECIPE"},
                rid, "unknown research status")
        for key in ("resetNotes", "access", "readEffects"):
            text(item[key], f"{rid}.{key}")
        citations(item["sources"], rid + ".sources")
        refs(item["gates"], gates, rid + ".gates")
        main = address(item, rid)
        primary[main] = item["name"]
        aliases = [address(alias, rid + ".aliases") for alias in
                   sequence(item["aliases"], rid + ".aliases")]
        for entry in [main, *aliases]:
            require(entry not in occupied, rid, f"address alias collision with {occupied.get(entry)}")
            occupied[entry] = rid
        if main[0] == "C22":
            expected = [] if main[2] in {0x0D, 0x0E} else [("MMD", 31, main[2])]
            require(aliases == expected, rid,
                    "C22 alias must explicitly match DEVAD 31, except direct-only REGCR/ADDAR")
        else:
            require(not aliases, rid,
                    "MMD aliases require an explicit evidence/coverage update")
        fields = sequence(item["fields"], rid + ".fields")
        if item["status"] in {"PENDING", "RECIPE"}:
            require(not fields and all(item[key] is None for key in
                    ("resetValue", "resetKnownMask", "reservedMask")), rid,
                    "pending/recipe records must retain unknown masks and no field claims")
            if item["status"] == "RECIPE":
                require(item["name"] in DIAGNOSTIC_RECIPES, rid,
                        "recipe-only completion requires an explicit coverage update")
                require(any(a["register"] == rid for a in actions.values()), rid,
                        "recipe-only record requires an exact write action")
        else:
            reset = hex_value(item["resetValue"], rid + ".resetValue")
            known = hex_value(item["resetKnownMask"], rid + ".resetKnownMask")
            used = hex_value(item["reservedMask"], rid + ".reservedMask")
            require(reset & ~known == 0, rid, "reset value sets an unknown reset bit")
            for field in fields:
                record(field, rid + ".field", "id name mask access legal prohibited neutral "
                       "readEffects validity sources owner")
                fid = identify(field, rid + ".field")
                for key in ("name", "readEffects", "validity"):
                    text(field[key], f"{fid}.{key}")
                mask = hex_value(field["mask"], fid + ".mask")
                require(mask != 0, fid, "field mask must not be zero")
                require(mask & used == 0, fid, "field overlaps another field or reserved bits")
                used |= mask
                unshifted = mask // (mask & -mask)
                require(unshifted & (unshifted + 1) == 0, fid, "field mask must be contiguous")
                require(isinstance(field["access"], str) and field["access"] in ACCESS,
                        fid, "unknown field access type")
                prohibited = [hex_value(v, fid + ".prohibited", unshifted) for v in
                              sequence(field["prohibited"], fid + ".prohibited")]
                require(len(set(prohibited)) == len(prohibited), fid,
                        "duplicate prohibited encoding")
                if field["legal"] != "range":
                    legal = [hex_value(v, fid + ".legal", unshifted) for v in
                             sequence(field["legal"], fid + ".legal", nonempty=True)]
                    require(len(set(legal)) == len(legal), fid, "duplicate legal encoding")
                    require(not set(legal) & set(prohibited), fid,
                            "encoding is both legal and prohibited")
                else:
                    require(len(prohibited) < unshifted + 1, fid,
                            "field has no legal encoding")
                neutral = field["neutral"]
                if field["access"] in {"W0C", "W1C", "W1S"}:
                    actual = hex_value(neutral, fid + ".neutral", unshifted)
                    require(actual == (unshifted if field["access"] == "W0C" else 0),
                            fid, "incorrect command-neutral encoding")
                    require(actual not in prohibited and
                            (field["legal"] == "range" or actual in legal), fid,
                            "command-neutral encoding must be legal")
                elif rid == "CDCR" and mask == 0x8000 and field["access"] == "RW":
                    require(hex_value(neutral, fid + ".neutral") == 0, fid,
                            "CDCR start command-neutral encoding must be zero")
                    require(0 not in prohibited and (field["legal"] == "range" or 0 in legal),
                            fid, "command-neutral encoding must be legal")
                else:
                    require(neutral is None, fid, "ordinary field neutral must be null")
                citations(field["sources"], fid + ".sources")
                text(field["owner"], fid + ".owner")
                require(field["owner"] in features, fid, "unknown feature owner")
                require(rid in features[field["owner"]]["registers"], fid,
                        "field owner must include the register in its coverage")
            require(used == 0xFFFF, rid, "field/reserved partition does not cover all 16 bits")
        if item["name"] in CORE:
            require(item["status"] == "COMPLETE" and item["researchStage"] == 3,
                    rid, "core record must have completed stage-3 research")
        if item["name"] in DIAGNOSTIC_REGISTERS | DIAGNOSTIC_RECIPES:
            expected_status = "RECIPE" if item["name"] in DIAGNOSTIC_RECIPES else "COMPLETE"
            require(item["status"] == expected_status and item["researchStage"] == 4,
                    rid, "diagnostic record must have completed stage-4 research")

    for offset, name in C22_INVENTORY.items():
        require(primary.get(("C22", None, offset)) == name, "coverage",
                f"missing or renamed C22 inventory register {name}")
    for offset, name in MMD_INVENTORY.items():
        require(primary.get(("MMD", 31, offset)) == name, "coverage",
                f"missing or renamed MMD inventory register {name}")
    for offset in RECIPE_OFFSETS:
        require(("MMD", 31, offset) in primary, "coverage",
                f"missing feature-only vendor offset 0x{offset:04X}")
    for offset, name in ((0x0122, "LOOP_0122"), (0x0123, "RECIPE_0123"), (0x0130, "LOOP_0130")):
        require(primary.get(("MMD", 31, offset)) == name, "coverage",
                "diagnostic recipe address/name mapping changed")
    permitted = ({("C22", None, offset) for offset in C22_INVENTORY}
                 | {("MMD", 31, offset) for offset in MMD_INVENTORY}
                 | {("MMD", 31, offset) for offset in RECIPE_OFFSETS})
    # Printed EEE addresses get their more specific unresolved-namespace error
    # below; all other additions require an explicit source/coverage update.
    printed_candidates = {("MMD", *pair) for pair in PRINTED_ADDRESSES}
    require(set(primary) <= permitted | printed_candidates, "coverage",
            "undocumented primary address requires an explicit evidence/coverage update")
    require(CORE <= names, "coverage", "missing core register")
    require(DIAGNOSTIC_REGISTERS | DIAGNOSTIC_RECIPES <= names, "coverage",
            "missing or renamed diagnostic register")
    for fid, stage in FEATURE_STAGES.items():
        require(fid in features, "coverage", f"missing feature family {fid}")
        require(features[fid]["researchStage"] == stage, fid,
                f"feature research belongs to prompt {stage}")
    for pid, fid in CORE_PROCEDURES.items():
        require(pid in procedures, "coverage", f"missing core procedure {pid}")
        require(procedures[pid]["feature"] == fid, pid,
                "core procedure has the wrong feature owner")
    for pid, fid in DIAGNOSTIC_PROCEDURES.items():
        require(pid in procedures, "coverage", f"missing diagnostic procedure {pid}")
        require(procedures[pid]["feature"] == fid, pid,
                "diagnostic procedure has the wrong feature owner")

    referenced_registers = set()
    for fid, item in features.items():
        text(item["name"], fid + ".name")
        integer(item["researchStage"], fid + ".researchStage", 3, 5)
        for stage in sequence(item["implementationStages"], fid + ".implementationStages",
                              nonempty=True):
            integer(stage, fid + ".implementationStages", 1, 80)
        owner(item["owner"], fid + ".owner")
        require(isinstance(item["scope"], str) and
                item["scope"] in {"SOFTWARE", "HARDWARE_ONLY", "OUTSIDE_PHY", "ABSENT"},
                fid, "unknown feature scope")
        referenced_registers.update(refs(item["registers"], registers, fid + ".registers"))
        refs(item["procedures"], procedures, fid + ".procedures")
        refs(item["gates"], gates, fid + ".gates")
        text(item["notes"], fid + ".notes")
        evidence = record(item["evidence"], fid + ".evidence",
                          "documented implemented hostTested sdkBuilt hardwareQualified")
        for key in ("documented", "implemented", "hostTested", "sdkBuilt", "hardwareQualified"):
            require(isinstance(evidence[key], str) and evidence[key] in EVIDENCE,
                    f"{fid}.evidence.{key}", "unknown evidence state")
        if item["researchStage"] <= 4:
            require(evidence["documented"] == "COMPLETE", fid,
                    "completed research stage requires documented coverage")
            require(item["scope"] != "SOFTWARE" or bool(item["procedures"]),
                    fid, "completed core software feature requires a procedure")
        for pid in item["procedures"]:
            require(procedures[pid]["feature"] == fid, fid,
                    "procedure ownership differs from feature backlink")
    require(referenced_registers == set(registers), "coverage",
            "every register needs at least one feature owner")

    require(set(actions) == set(WRITE_ACTIONS), "coverage",
            "write actions must match the exact sourced action inventory")
    for aid, item in actions.items():
        rid = text(item["register"], aid + ".register")
        require(rid in registers, aid, "unknown action register")
        fid = text(item["feature"], aid + ".feature")
        require(fid in features and rid in features[fid]["registers"], aid,
                "write action is outside feature coverage")
        mask = hex_value(item["mask"], aid + ".mask")
        value = hex_value(item["value"], aid + ".value")
        require(mask != 0 and not value & ~mask, aid, "action value exceeds its nonzero mask")
        if item["neutral"] is not None:
            require(not hex_value(item["neutral"], aid + ".neutral") & ~mask,
                    aid, "action neutral exceeds mask")
        for key in ("effect", "prerequisites"):
            text(item[key], aid + "." + key)
        citations(item["sources"], aid + ".sources")
        refs(item["gates"], gates, aid + ".gates")
        require(registers[rid]["status"] in {"COMPLETE", "RECIPE"}, aid,
                "write action requires researched register")
        neutral = None if item["neutral"] is None else hex_value(item["neutral"], aid + ".neutral")
        require((rid, mask, value, neutral, fid) == WRITE_ACTIONS[aid], aid,
                "write action differs from exact sourced allowance")
        required_gate = "bist_result_semantics" if rid == "BICSR1" else "loopback_recipe"
        require(required_gate in item["gates"], aid, "write action must retain its evidence gate")

    used_actions = set()
    for pid, item in procedures.items():
        text(item["feature"], pid + ".feature")
        require(item["feature"] in features, pid, "unknown procedure feature")
        require(pid in features[item["feature"]]["procedures"], pid,
                "procedure missing from owning feature")
        modes(item["modes"], pid + ".modes")
        citations(item["sources"], pid + ".sources")
        refs(item["gates"], gates, pid + ".gates")
        owner(item["owner"], pid + ".owner")
        action_refs = refs(item.get("writeActions", []), actions, pid + ".writeActions")
        if pid in PROCEDURE_ACTIONS:
            require(set(action_refs) == PROCEDURE_ACTIONS[pid], pid,
                    "required procedure write actions are missing or changed")
        for aid in action_refs:
            require(actions[aid]["feature"] == item["feature"], pid,
                    "write action ownership differs from procedure")
            used_actions.add(aid)
        for key in ("prerequisites", "steps", "timeout", "partialEffects", "cleanup", "restoration"):
            contract(item[key], f"{pid}.{key}")
        sequence(item["steps"], pid + ".steps", nonempty=True)
        contract(item["waits"], pid + ".waits", allow_empty=True)
        seen_registers = set()
        for entry in sequence(item["ownedMasks"], pid + ".ownedMasks"):
            record(entry, pid + ".ownedMasks", "register mask")
            rid = text(entry["register"], pid + ".ownedMasks.register")
            require(rid in registers, pid, "unknown owned register")
            require(rid in features[item["feature"]]["registers"], pid,
                    "owned register is missing from feature coverage")
            require(rid not in seen_registers, pid, "duplicate owned register mask")
            seen_registers.add(rid)
            require(registers[rid]["status"] == "COMPLETE", pid,
                    "owned register fields have not been researched")
            mask = hex_value(entry["mask"], pid + ".ownedMasks.mask")
            reserved = hex_value(registers[rid]["reservedMask"], rid + ".reservedMask")
            require(mask != 0 and not mask & reserved, pid,
                    "owned mask includes reserved bits or is empty")
            writable = 0
            for field in registers[rid]["fields"]:
                if field["access"] in {"RW", "RHW", "W1S", "W0C", "W1C"}:
                    writable |= hex_value(field["mask"], rid + ".field.mask")
            require(not mask & ~writable, pid, "owned mask includes read-only fields")
    require(used_actions == set(actions), "coverage", "every write action needs a procedure")

    printed = entries("printedAddresses", "devad printedAddress sources gates feature")
    tuples = set()
    for aid, item in printed.items():
        pair = (hex_value(item["devad"], aid + ".devad", 31),
                hex_value(item["printedAddress"], aid + ".printedAddress"))
        require(pair not in tuples, aid, "duplicate printed address")
        tuples.add(pair)
        citations(item["sources"], aid + ".sources")
        refs(item["gates"], gates, aid + ".gates", nonempty=True)
        text(item["feature"], aid + ".feature")
        require(item["feature"] in features, aid, "unknown printed-address feature")
        require(("MMD", *pair) not in occupied, aid,
                "unresolved printed address is incorrectly claimed as a wire offset")
    require(PRINTED_ADDRESSES <= tuples, "coverage", "missing printed EEE address")

    identity = record(data["identity"], "identity", "oui id1 ouiLowMask modelMask revisionMask "
                      "models documentedRevision combinedIds policy sources")
    citations(identity["sources"], "identity.sources")
    text(identity["policy"], "identity.policy")
    hex_value(identity["oui"], "identity.oui", 0xFFFFFF)
    for key in ("id1", "ouiLowMask", "modelMask", "revisionMask"):
        hex_value(identity[key], "identity." + key)
    hex_value(identity["documentedRevision"], "identity.documentedRevision", 15)
    for key, maximum in (("models", 63), ("combinedIds", 0xFFFFFFFF)):
        record(identity[key], "identity." + key, "BASIC ENHANCED")
        for mode in ("BASIC", "ENHANCED"):
            hex_value(identity[key][mode], f"identity.{key}.{mode}", maximum)
    for index, item in enumerate(sequence(data["timings"], "timings", nonempty=True)):
        location = f"timings[{index}]"
        record(item, location, "id value unit qualifier start end conditions sources")
        identify(item, location)
        citations(item["sources"], location + ".sources")
        # Compare integers directly: converting an arbitrarily large JSON integer
        # to float can raise OverflowError. This is an encoding bound, not a
        # silicon timing limit; all values must fit finite binary64 magnitude.
        require(type(item["value"]) in (int, float)
                and 0 <= item["value"] <= sys.float_info.max, location,
                "timing value must be finite and nonnegative within binary64 range")
        for key in ("unit", "qualifier", "start", "end", "conditions"):
            text(item[key], f"{location}.{key}")
        require(re.search(r"\b(MIN|NOM|MAX)\b", item["qualifier"]) is not None,
                location, "timing needs an explicit MIN, NOM or MAX qualifier")

    pause = record(data["pauseResolution"], "pauseResolution", "fields phycrRxTx eligibility sources")
    citations(pause["sources"], "pauseResolution.sources")
    text(pause["eligibility"], "pauseResolution.eligibility")
    fields = sequence(pause["fields"], "pauseResolution.fields")
    require(len(fields) == 4, "pauseResolution.fields", "expected four pause encodings")
    encodings = [hex_value(v, "pauseResolution.fields") for v in fields]
    require(len(set(encodings)) == 4 and all(v & ~0x0C00 == 0 for v in encodings),
            "pauseResolution.fields", "invalid or duplicate pause encoding")
    matrix = sequence(pause["phycrRxTx"], "pauseResolution.phycrRxTx")
    require(len(matrix) == 4, "pauseResolution.phycrRxTx", "expected four matrix rows")
    for row in matrix:
        sequence(row, "pauseResolution.phycrRxTx.row")
        require(len(row) == 4, "pauseResolution.phycrRxTx.row", "expected four matrix columns")
        for value in row:
            require(hex_value(value, "pauseResolution.phycrRxTx.value") & ~0x3000 == 0,
                    "pauseResolution.phycrRxTx", "result contains non-pause bits")


def load_ledger(path: Path) -> object:
    """Read strict JSON; reject duplicate object keys instead of hiding records."""
    def unique_object(pairs: list[tuple[str, object]]) -> dict:
        result = {}
        for key, value in pairs:
            if key in result:
                raise LedgerError(f"duplicate JSON object key {key}")
            result[key] = value
        return result

    def reject_constant(value: str) -> None:
        raise LedgerError(f"non-JSON numeric constant {value}")

    return json.loads(path.read_text(encoding="utf-8"), object_pairs_hook=unique_object,
                      parse_constant=reject_constant)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("ledger", nargs="?", type=Path, default=DEFAULT_LEDGER)
    args = parser.parse_args(argv)
    try:
        data = load_ledger(args.ledger)
        validate(data)
    except (OSError, UnicodeError, ValueError, TypeError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        return 1
    print(f"PASS: {len(data['registers'])} registers, {len(data['procedures'])} procedures, "
          f"{len(data['features'])} features; structure and coverage only")
    return 0


if __name__ == "__main__":
    sys.exit(main())
