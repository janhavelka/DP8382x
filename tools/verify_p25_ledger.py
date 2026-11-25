"""Check the SNLS638C inventory and the named owners of its field/procedure rows."""

from __future__ import annotations

import re


# Table 7-1. The six printed IEEE headings have distinct low wire offsets.
DIRECT = set(range(32)) - {0x1D}
VENDOR = {
    0x25, 0x27, 0x101, 0x10A, 0x123, 0x130, *range(0x170, 0x179),
    *range(0x180, 0x18B), 0x302, 0x305, 0x308, 0x30B, 0x30C, 0x30F,
    0x311, 0x313, 0x31C, 0x31F, 0x33C, 0x33E, 0x404, 0x40D, 0x416,
    0x428, 0x429, 0x456, 0x460, 0x461, 0x467, 0x468, 0x469,
    *range(0x4A0, 0x4A5), *range(0x4CD, 0x4D3), *range(0x4D4, 0x4D8),
}
IEEE = {0x1000: (3, 0), 0x1001: (3, 1), 0x1014: (3, 20), 0x1016: (3, 22),
        0x203C: (7, 60), 0x203D: (7, 61)}
EXCEPTIONS = {0x42, 0x218, 0x30E, 0x333, 0x31B, 0x33A, 0x33D, 0x4A5, 0x4A6, 0x4A7}
CORE = set("""identify mdio external_reset pcs_reset hard_reset restart initialize
    clock_role rmii follower_timing clock_loss configure_an configure_forced next_page
    observe_link fast_drop link_options repeater""".split())
FEATURE = set("""functional_mode eee legacy_lpi power irq counters quality loopback bist
    tdr led_pin wake vod analog compliance""".split())


def require(ok: bool, where: str, reason: str) -> None:
    if not ok:
        raise ValueError(f"DP83825I {where}: {reason}")


def word(cell: str) -> int:
    require(bool(re.fullmatch(r"[0-9A-F]{4}", cell)), str(cell), "expected four hex digits")
    return int(cell, 16)


def validate(data: dict) -> None:
    """Validate the source ledger without requiring a PDF parser or silicon."""
    require(data["variant"] == "DP83825I", "variant", "exact declaration required")
    procedures = data["procedures"]
    expected = CORE | FEATURE
    require(len(procedures) == len(expected) and {p["id"] for p in procedures} == expected,
            "procedures", "missing, duplicated or unknown procedure")
    for p in procedures:
        require(p["owner"]["implementationStage"] == (60 if p["id"] in CORE else 61),
                p["id"], "incorrect implementation stage")
        require(all(isinstance(p.get(k), str) and p[k].strip() for k in ("source", "contract", "vector")),
                p["id"], "missing source, contract or test vector")

    registers = data["registers"]
    offsets = [word(reg["offset"]) for reg in registers]
    require(offsets == sorted(DIRECT | VENDOR | IEEE.keys()), "table 7-1", "address inventory changed")
    count = named_reserved = prose_reserved = 0
    for index, (reg, offset) in enumerate(zip(registers, offsets), 1):
        location = reg["offset"]
        require((reg["section"], reg["table"]) == (f"7.{index}", f"7-{index+2}"),
                location, "detailed table citation changed")
        wire = IEEE.get(offset, (None if offset in DIRECT else 31, offset))
        require(reg["space"] == ("C22" if offset in DIRECT else "MMD") and
                (reg["devad"], word(reg["wireOffset"])) == wire, location, "wrong wire address")
        mask = 0
        for field in reg["fields"]:
            bounds = field["bits"].split("-")
            high, low = int(bounds[0]), int(bounds[-1])
            require(len(bounds) <= 2 and 0 <= low <= high <= 15, location, "invalid bit range")
            width = high - low + 1
            part = ((1 << width) - 1) << low
            require(not mask & part, location, "overlapping fields")
            mask |= part
            require(field["access"] in {"R", "W", "R/W", "RC"}, location, "unknown access")
            require(re.fullmatch(r"[0-9A-F]+h", field["reset"]) is not None and
                    int(field["reset"][:-1], 16) < 1 << width, location, "invalid reset cell")
            use = field["use"]
            valid = expected | {"reserved_by_description"}
            if field["name"] == "RESERVED":
                valid = {"reserved"}
                named_reserved += 1
            require(use in valid, location, "unknown or incorrectly exposed field")
            prose_reserved += use == "reserved_by_description"
            codes = field.get("codes", [])
            require(len(codes) == len(set(codes)) and
                    all(type(c) is int and 0 <= c < 1 << width for c in codes),
                    location, "invalid enumerated encoding")
            count += 1
        require(mask == 0xFFFF, location, "field partition incomplete")
    require((count, named_reserved, prose_reserved) == (570, 201, 8), "fields", "row counts changed")

    rows = data["recipeOnly"]
    require(len(rows) == len(EXCEPTIONS) and {word(r["wireOffset"]) for r in rows} == EXCEPTIONS,
            "exceptions", "undocumented or missing exception")
    for r in rows:
        require(r["devad"] == 31 and r["procedure"] in expected and r["source"] and r["action"],
                r["wireOffset"], "incomplete procedure-only authority")
    modes = data["modeRecipes"]
    require(set(modes) == {"powerOptimized", "reachOptimized", "reachWithEee"},
            "recipes", "incorrect functional modes")
    for name, recipe in modes.items():
        require(len(recipe) == (32 if name == "reachWithEee" else 13)
                and recipe[-1] == ["001F", "4000"], name, "truncated or extended mode sequence")
        for a, v in recipe[:-1]:
            require(word(a) in VENDOR | {0x31B, 0x33A, 0x33D}, name, "unauthorized register")
            word(v)
