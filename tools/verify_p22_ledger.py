"""Offline structural and literal checks for the SNLS505H research ledger."""

from __future__ import annotations

import re


# Original table headings 8-1 to 8-142 in SNLS505H, pp. 53–102.
TABLE_ADDRESSES = (
    *range(0x0C), *range(0x0D, 0x1D), 0x1E, 0x1F,
    0x25, 0x27, 0x3E, 0x3F, 0x40, 0x42, 0x101, 0x106, 0x107, 0x10F,
    0x111, 0x114, 0x116, 0x121, 0x122, 0x123, 0x126, 0x129, 0x130,
    0x155, 0x170, 0x171, 0x173, 0x177, *range(0x180, 0x18B), 0x215,
    0x21D, 0x403, 0x404, 0x40D, 0x410, 0x416, 0x418, 0x41F, 0x421,
    0x428, 0x450, 0x456, 0x460, 0x461, 0x462, 0x463, 0x465, 0x467,
    0x468, 0x469, *range(0x4A0, 0x4CD), 0x4D0, 0x4D1, *range(0x4D4, 0x4D7),
    0x3000, 0x3001, 0x3014, 0x3016, 0x703C, 0x703D,
)
# No field is printed for these intervals; table 8-39 does print a reserved row.
UNLISTED = {(0x000B, 11, 11), (0x0101, 6, 0)}
# Named R/W fields whose independent use is constrained by selector ownership,
# recipe scope or missing programming rules. Reserved lanes are checked separately.
PROTECTED_MASKS = {
    0x000D: 0xC01F, 0x000E: 0xFFFF, 0x003F: 0x00F0, 0x0101: 0x0080,
    0x0106: 0xF4CF, 0x010F: 0x0100, 0x0111: 0x000F, 0x0126: 0x7FFF,
    0x0129: 0x00FF, 0x0130: 0x0800, 0x0171: 0x000F, 0x0173: 0xFF00,
    0x0177: 0x1F00, 0x021D: 0x0FFF, 0x040D: 0x001F, 0x0416: 0x1F00,
    0x0418: 0x3F00, 0x04A0: 0x0040,
}
FIELD_ACCESS = {
    "R", "R, RH", "R, RC", "R, RC, RH/W1S", "RC/W", "R/W", "R/W, RH",
    "R/W, W1S", "RH/W1S", "W, RH/W1S", "UNLISTED",
}
PROCEDURE_STAGES = {
    63: "identity straps management reset copper-link negotiation mdix link-loss irq counters",
    64: "mac-interface clock-phase",
    65: "fiber-mode fiber-fault fiber-drive",
    66: "loopback bist tdr alcd quality dsp cable-break-profile electrical compliance repeater",
    67: "power eee lpi wol led-gpio sfd",
}


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def word(value: str, where: str) -> int:
    require(isinstance(value, str) and re.fullmatch(r"0x[0-9A-F]{4}", value) is not None,
            f"{where}: expected a 16-bit hexadecimal word")
    return int(value, 16)


def check_parts(parts: list) -> None:
    # Section 4, the ordering addendum, and the common identity in tables 8-3/4.
    expected = {"DP83822I": (85, False), "DP83822H": (125, False),
                "DP83822IF": (85, True), "DP83822HF": (125, True)}
    require(len(parts) == 4 and {p["part"] for p in parts} == set(expected),
            "parts: require four distinct ordering declarations")
    for part in parts:
        name = part["part"]
        high, fiber = expected[name]
        require(part["minTempC"] == -40 and part["maxTempC"] == high and part["fiber"] is fiber,
                f"{name}: incorrect grade or FX applicability")
        require(part["idWords"] == ["0x2000", "0xA240"] and part["oui"] == "0x080028"
                and part["model"] == "0x24" and type(part["revision"]) is int and part["revision"] == 0
                and part["observedMode"] == "UNKNOWN", f"{name}: incorrect observed identity")
        require(part["package"] == "32-pin VQFN RHB" and
                set(part["copperInterfaces"]) == {"MII", "RMII", "RGMII"} and
                set(part["fxInterfaces"]) == ({"MII", "RMII"} if fiber else set()),
                f"{name}: incorrect package or source-admitted MAC interface")
        require(part["admission"] and part["physical"], f"{name}: missing part-specific evidence")


def check_encoding(encoding: dict) -> None:
    # Section 7.4.2.9, tables 8-11/23 and the wake filter tables 8-87, 8-95..131.
    expected = {
        "dataFunction": 1, "directOnly": ["0x000D", "0x000E"],
        "mmd7ReadSelectors": ["0x0007", "0x003D", "0x4007"],
        "rgmiiShiftNs": 3.5, "rgmiiShiftWords": ["0x0800", "0x1800", "0x0000", "0x1000"],
        "fxEnableMask": "0x4000", "fxEnableValue": "0x4000",
        "patternByteCount": 64, "patternMaskIgnoredBit": 1, "patternOffsetReset": 12,
        "wakeAckMask": "0x0800",
    }
    for key, literal in expected.items():
        value = encoding.get(key)
        require(type(value) is type(literal) and value == literal,
                f"encoding.{key}: differs from the source literal")


def check_register(reg: dict, number: int, address: int, owners: dict) -> tuple[int, int]:
    label = f"table 8-{number} (0x{address:04X})"
    require(type(reg["table"]) is int and reg["table"] == number and
            word(reg["printedAddress"], label) == address, f"{label}: incorrect printed address or table")
    space, devad, offset = "standard", None, address
    if address > 0xFFF:
        space, devad, offset = "mmd", address >> 12, address & 0xFFF
    elif address > 31:
        space, devad = "vendor", 31
    require((reg["space"], reg["devad"], word(reg["offset"], label)) == (space, devad, offset),
            f"{label}: wrong wire namespace or offset")
    ordinary = word(reg["ordinaryWriteMask"], label)
    covered = 0
    owned = 0
    protected = 0
    # SNLS505H section 7.4.9 overlays a freeze-and-clear write on a count field.
    neutral = 0x8000 if address == 0x001B else 0
    collector = address in {0x001A, 0x001B}  # Prose adds sticky/capture semantics.
    reserved = 0
    for field in reg["fields"]:
        high, low = field["msb"], field["lsb"]
        require(type(high) is int and type(low) is int and 0 <= low <= high < 16,
                f"{label}: invalid field bounds")
        mask = ((1 << (high - low + 1)) - 1) << low
        require(not (covered & mask), f"{label}: overlapping field {high}:{low}")
        covered |= mask
        location = f"{label}[{high}:{low}]"
        name, access = field["name"], field["access"]
        require(isinstance(name, str) and name.strip() and access in FIELD_ACCESS,
                f"{location}: missing name or unknown access")
        unlisted = (address, high, low) in UNLISTED
        require((name == "UNLISTED") == unlisted, f"{location}: incorrect unlisted range")
        # Table 8-38 omits TYPE for a named reserved row; its reset is printed.
        require((access == "UNLISTED") == (unlisted or (address, high, low) == (0x106, 11, 11)),
                f"{location}: incorrect missing TYPE disposition")
        reset = field["reset"]
        reset_valid = reset is None if unlisted else isinstance(reset, str) and reset.strip()
        require(reset_valid, f"{location}: missing or invented default cell")
        inactive = unlisted or name.casefold() == "reserved"
        policy = field["writePolicy"]
        if inactive:
            require(field["procedure"] is None and policy == ("unlisted" if unlisted else "reserved"),
                    f"{location}: exposed a reserved or unlisted field")
            reserved += not unlisted
        else:
            require(field["procedure"] in owners, f"{location}: field has no assigned procedure")
            require(isinstance(field["description"], str) and field["description"].strip(),
                    f"{location}: missing field disposition")
            require(policy in {"owned-setting", "read-only", "pulse", "protected"},
                    f"{location}: unknown field write policy")
        if policy == "owned-setting":
            require(access in {"R/W", "R/W, RH", "RC/W"}, f"{location}: exposed protected access")
            owned |= mask
        elif policy == "protected":
            protected |= mask
        # Table 8-88 puts this event access on the entire RXFS low byte,
        # including reserved bits. Never echo a sampled event into a write.
        if policy == "pulse" or access == "R, RC, RH/W1S":
            neutral |= mask
        collector |= "RC" in access
    require(covered == 0xFFFF, f"{label}: field inventory incomplete")
    require(owned == ordinary, f"{label}: ordinary write mask disagrees with field access")
    require(protected == PROTECTED_MASKS.get(address, 0), f"{label}: protected field policy changed")
    require(word(reg["neutralZeroMask"], label) == neutral and not (ordinary & neutral),
            f"{label}: incorrect command/status neutral mask")
    require(reg["collector"] is collector, f"{label}: wrong destructive-read ownership")
    return len(reg["fields"]), reserved


def check_recipes(recipes: list, owners: dict) -> None:
    # Section 6.20/.21 footnotes, section 7.3.3, table 8-23 and SNLA437A section 2.5.2.
    exceptions = {
        "fast-rxdv-cr1": (0x0009, 0x0002, "clock-phase", 64),
        "rgmii-fast-rxdv": (0x0457, 0x0410, "clock-phase", 64),
        "rmii-entry": (0x0462, 0x4300, "mac-interface", 64),
        "sfd-timing": (0x0456, 0x000A, "sfd", 67),
        "analog-prbs-lock": (0x030B, 0x3380, "bist", 66),
        "bist-capture-example": (0x001B, 0x807D, "bist", 66),
    }
    # Independent transcription of SNLA383 section 3.1, including the optional
    # EEE step and the two different PHYRCR words at either end of the trace.
    cable_break = (
        (0x001F, 0x8000), (0x000B, 0x120F), (0x0101, 0x2082), (0x0106, 0xD4FD),
        (0x0107, 0x0605), (0x0126, 0x462B), (0x04D4, 0x3322), (0x0121, 0x0A00),
        (0x0122, 0x0400), (0x0123, 0x0200), (0x010F, 0x0100), (0x0111, 0x6009),
        (0x0129, 0x009F), (0x0130, 0x470A), (0x0410, 0x6000), (0x0416, 0x1070),
        (0x0418, 0x3F00), (0x0450, 0x2141), (0x003F, 0xB40F), (0x040D, 0x000E),
        (0x04D1, 0x0182), (0x001F, 0x4000),
    )
    sequence = []
    for position, (address, value) in enumerate(cable_break, 1):
        name = f"cable-break-{position:02d}"
        exceptions[name] = (address, value, "cable-break-profile", 66)
        sequence.append(name)
    names = [r["id"] for r in recipes]
    require(len(names) == len(exceptions) and set(names) == exceptions.keys(),
            "recipes: missing, extra or duplicated source exception")
    require([n for n in names if n.startswith("cable-break-")] == sequence,
            "recipes: SNLA383 cable-break sequence out of order")
    for recipe in recipes:
        name = recipe["id"]
        address, value = word(recipe["address"], name), word(recipe["value"], name)
        proc = recipe["procedure"]
        require(recipe["ordinaryWrite"] is False and (address, value, proc, owners[proc]) == exceptions[name]
                and recipe["scope"] and recipe["source"], f"{name}: incorrect scoped recipe or owner")
        if proc == "cable-break-profile":
            require(recipe["optional"] is (name == "cable-break-21"), f"{name}: wrong optionality")
        if name == "sfd-timing":
            require(recipe["interfaces"] == ["MII"], "sfd-timing: 120 ns IPG requires MII")


def check_source_decisions(data: dict, owners: dict) -> None:
    resets = data["resetDomains"]
    domains = {
        "power-pin": ("POR or RESET_N", "all", "relatch"),
        "phyrcr-hard": ("C22 001F=8000", "all", "relatch"),
        "phyrcr-restart": ("C22 001F=4000", "digital restart; registers retained", "retain"),
        "bmcr-pcs": ("C22 0000[15]=1", "PCS; vendor registers retained", "retain"),
        "mmd-pcs": ("DEVAD 3 offset 0000[15]=1", "MMD3, MMD7 and PCS; vendor registers retained",
                    "no documented relatch"),
    }
    require(len(resets) == len(domains) and
            {r["id"]: (r["action"], r["effect"], r["strapAction"]) for r in resets} == domains,
            "resetDomains: incorrect reset mechanism, register domain or strap action")
    for reset in resets:
        require(reset["interpretation"] and reset["source"], f"{reset['id']}: missing reset disposition")
    timings = data["timings"]
    # The 6-1 note uses a different clock/reset edge pair from section 6.7 T1.
    bounds = {
        "6.6 T2": (200, "ms", "MAX", "power application", "SMI-ready for MDC preamble"),
        "6.6 T3": (200, "ms", "MAX", "power application", "hardware configuration latched"),
        "6.6 T4": (64, "ns", "TYP", "power-up configuration latch", "strap pins switch to outputs"),
        "6.6 T5": (1.5, "s", "TYP", "power application", "fast link pulse"),
        "6.7 T1": (1, "us", "MIN", "XI stable", "subsequent reset application"),
        "6.7 T2": (10, "us", "MIN", "RESET_N assertion", "RESET_N release"),
        "figure 6-1 note": (100, "us", "MIN", "external clock stable", "RESET_N release"),
        "6.8 T1": (10, "us", "MIN", "RESET_N assertion", "RESET_N release"),
        "6.8 T2": (2, "ms", "MAX", "RESET_N release", "SMI-ready for MDC preamble"),
        "6.8 T3": (120, "ns", "TYP", "RESET_N release", "strap latch"),
        "6.8 T4": (64, "ns", "TYP", "reset configuration latch", "strap pins switch to outputs"),
        "6.8 T5": (1.5, "s", "TYP", "RESET_N release", "fast link pulse"),
    }
    indexed = {t["source"]: t for t in timings}
    require(len(indexed) == len(timings), "timings: duplicate source row")
    for citation, expected in bounds.items():
        row = indexed["SNLS505H " + citation]
        require(tuple(row[k] for k in ("value", "unit", "qualifier", "start", "end")) == expected
                and row["conditions"], f"{citation}: incorrect timing bound or reference edges")
    tx = ("TX_CLK rising with TX_EN asserted", "start of MDI /J/ symbol")
    rx = ("start of MDI /J/ symbol", "RX_CLK rising with RX_DV asserted")
    latencies = [
        ("6.20 T1", "MII", 48, 56, "no FAST_RXDV footnote", tx),
        ("6.20 T1", "RMII", 102, None, "FAST_RXDV footnote 1", tx),
        ("6.20 T1", "RGMII", 170, None, "FAST_RXDV footnote 2", tx),
        ("6.21 T2", "MII", 194, 218, "FAST_RXDV footnote 1", rx),
        ("6.21 T2", "RMII", 272, None, "FAST_RXDV footnote 1", rx),
        ("6.21 T2", "RGMII", 159, None, "FAST_RXDV footnote 2", rx),
    ]
    keys = {"SNLS505H " + citation for citation in bounds}
    for citation, interface, typical, maximum, footnote, edges in latencies:
        for qualifier, value in (("TYP", typical), ("MAX", maximum)):
            if value is None:
                continue
            source = f"SNLS505H {citation} {interface} {qualifier}"
            keys.add(source)
            row = indexed[source]
            expected = (value, "ns", qualifier, *edges, f"100BASE-TX {interface}, {footnote}")
            require(tuple(row[k] for k in ("value", "unit", "qualifier", "start", "end", "conditions"))
                    == expected, f"{source}: incorrect latency or measurement conditions")
    require(keys == indexed.keys(), "timings: unrecognized timing row")
    gates = data["gates"]
    required = set("""reset-selection regcr-functions rmii-transition phase-width fast-rxdv-profile
        fiber-interface fiber-observation fx-control counter-threshold stray-led-name timers-and-raw-fields
        analog-bist tdr-amplitude alcd-scale cable-break-restore harmonics power-exit wol-bit-mask
        ptp-default source-gaps eee-control wol-mac-order misr-enable-retention""".split())
    ids = {g["id"] for g in gates}
    require(len(ids) == len(gates) and required <= ids, "gates: missing or duplicated conflict")
    for gate in gates:
        require(gate["procedure"] in owners and all(isinstance(gate[k], str) and gate[k].strip()
                for k in ("source", "conflict", "decision", "nextEvidence")),
                f"{gate['id']}: incomplete source decision or evidence request")


def verify(data: dict) -> list[str]:
    """Return a diagnostic on invalid research records. Requires only the standard library."""
    try:
        check_parts(data["parts"])
        check_encoding(data["encoding"])
        procedures = data["procedures"]
        owners = {p["id"]: p["owner"]["implementationStage"] for p in procedures}
        expected = {name: stage for stage, group in PROCEDURE_STAGES.items() for name in group.split()}
        require(len(owners) == len(procedures) and owners == expected,
                "procedures: missing, misassigned or duplicate procedure")
        parts = {part["part"] for part in data["parts"]}
        for proc in procedures:
            require(type(owners[proc["id"]]) is int and all(isinstance(proc[key], str) and proc[key].strip()
                    for key in ("scope", "source", "acceptance")), f"{proc['id']}: incomplete contract")
            applicable = {"DP83822IF", "DP83822HF"} if owners[proc["id"]] == 65 else parts
            require(set(proc["parts"]) == applicable and len(proc["parts"]) == len(applicable),
                    f"{proc['id']}: incorrect part applicability")
        registers = sorted(data["registers"], key=lambda r: r["table"])
        require(len(registers) == len(TABLE_ADDRESSES), "registers: incomplete table inventory")
        fields = reserved = 0
        for n, (reg, address) in enumerate(zip(registers, TABLE_ADDRESSES), 1):
            count, unused = check_register(reg, n, address, owners)
            fields += count
            reserved += unused
        require((fields, reserved) == (578, 144),
                "fields: require 576 printed rows (144 reserved) and two unlisted ranges")
        check_recipes(data["recipes"], owners)
        check_source_decisions(data, owners)
        reads = data["reads"]
        require(len(reads) == 1, "reads: expected one off-table MSE observation")
        mse = reads[0]
        require((mse["address"], mse["space"], mse["devad"], mse["procedure"]) ==
                ("0x0218", "vendor", 31, "quality") and mse["ordinaryWrite"] is False,
                "reads: invalid SNLA437A MSE address or access")
    except (KeyError, TypeError, ValueError) as exc:
        return [f"DP83822: {exc}"]
    return []
