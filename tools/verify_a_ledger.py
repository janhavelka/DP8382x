"""Check the AI/AE comparison against both the original ledger and the TI archive."""

from pathlib import Path
import re


SOURCE = Path(__file__).resolve().parents[1] / "docs/reference/text/TI-DP83826AI-AE-SNLS783A.pdf.txt"
HEADER = re.compile(
    r"^8\.5\.1\.(\d+) [^\n]+ \(Offset = ([0-9A-Fa-f]+)h\) \[Reset = ([0-9A-Fa-fXx]+)h\]", re.M)
# In the archived extract, field names may wrap or share a line with the access
# column. Tables 8-80/81 add an OTP column after the printed reset.
FIELD = re.compile(
    r"^(\d{1,2})(?:[-:](\d{1,2}))?[ \t]*\n([^\n]+(?:\n[^\n]+){0,2}?)\s+"
    r"(RH/W1S|RH/W|R/W|RCH|RC|RH|R|W0C)[ \t]*\n([0-9A-Fa-fXx]+h?)\b(?:[ \t]*\n(Y)\b)?", re.M)

# Numeric reset cells qualified by SNLS783A mode and strap descriptions.
# X cells and factory-programmed fields are excluded directly in the table scan.
VARIABLE_RESET_BITS = {
    "PHYIDR2": 0x0020,            # 8-23: models 11h/13h occupy bits 9:4.
    "CR3": 0x000F,                # 8-5/6: strap-selected fast-drop mechanisms.
    "PHYSTS": 0x0006,             # 8-35: speed/duplex when negotiation is disabled.
    "LED2_GPIO_CFG": 0x0007,      # 8-77: strap-selected pin mux.
    "CLK_OUT_LED_STATUS": 0x0001, # 8-10/79: Strap6 selects the pin 31 function.
    "SOR1": 0x0FFF,               # 8-88: sampled strap levels.
    "SOR2": 0x5FBF,               # 8-89: sampled strap configuration.
    "RXFCFG1": 0x0001,            # 8-91: magic-packet enable defaults from strap.
}

# Formerly reserved I/E bits named by SNLS783A tables 8-88 and 8-90.
ADDED_FIELDS = {
    "SOR1": {"SOR1.strap11": (0x0800, 58)},
    "LEDCFG2": {"LEDCFG2.led3_polarity": (0x0400, 59),
                "LEDCFG2.led3_drive_value": (0x0200, 59),
                "LEDCFG2.led3_drive_enable": (0x0100, 59)},
}

# Prompt 57 assigns these deltas or register reviews separately from the
# original feature family. The pin-31 clock selection is gated at table 8-76.
FIELD_OWNER_EXCEPTIONS = {
    "FLDS.causes": 58, "PHYSCR.interrupt_enable": 58, "PHYSCR.interrupt_output_enable": 58,
    "IO_CFG1.rx_dv": 59, "LED1_GPIO_CFG.clock_source": 59,
}

# Prompt 58 owns startup, interface and link. Prompt 59 owns diagnostic and
# optional controls. Keep the partition independent of the rows being checked.
CORE_FEATURES = {
    "identity", "access", "reset", "initialization", "negotiation", "next_page",
    "crossover", "link", "fast_drop", "link_recovery", "clock", "mii", "rmii",
    "repeater", "compatibility", "straps", "outside_phy",
}
CORE_GATES = {
    "forced_physts", "hard_reset_straps", "pcs_reset_extent", "rmii_rc",
    "fld_reset_defaults", "next_page_handshake", "fld_threshold_recipe",
}


def procedure_masks(base):
    """Apply only the SNLS783A mux and LED changes to the original ownership."""
    name = base["id"]
    masks = {r["register"]: r["mask"] for r in base["ownedMasks"]}
    if name == "clock_configure":
        return {"LED0_GPIO_CFG": "0x003F"}  # Table 8-75, mux and clock source.
    if name == "led_configure":
        masks.update(LEDCFG="0x0FFF", LEDCFG2="0x0777")  # Tables 8-86/90.
    if name in ("interrupt_configuration", "wol_configure", "pin_configure"):
        masks.update(LED0_GPIO_CFG="0x0007", LED3_GPIO_CFG="0x0007")
    if name == "pin_configure":
        del masks["IO_CFG1"]  # Table 8-74 now reserves bit 6.
    return masks


def require(condition, message):
    if not condition:
        raise ValueError(message)


def indexed(records, label, key="id"):
    result = {record[key]: record for record in records}
    require(len(result) == len(records), f"{label}: duplicate {key}")
    return result


def hex16(value):
    require(isinstance(value, str) and re.fullmatch(r"0x[0-9A-F]{4}", value),
            f"expected a 16-bit hex literal, got {value!r}")
    return int(value, 16)


def check_register(register, heading, table):
    name = register["id"]
    number, offset, header_reset = heading.groups()
    require(register["section"] == f"8.5.1.{number}" and register["table"] == f"8-{int(number) + 19}"
            and hex16(register["address"]) == int(offset, 16)
            and register["headerReset"] == header_reset, f"{name}: detailed table citation/address/reset")
    reserved = known = all_bits = literal = otp_bits = 0
    fields = []
    reserved_rows = []
    for high, low, field_name, access, reset, otp in FIELD.findall(table):
        bits = f"{high}-{low}" if low else high
        high, low = int(high), int(low or high)
        require(0 <= low <= high < 16, f"{name}: invalid source partition")
        mask = (2 ** (high - low + 1) - 1) << low
        require(not all_bits & mask, f"{name}: duplicate source bits")
        all_bits |= mask
        if "RESERVED" in field_name.upper():
            reserved |= mask
            reserved_rows.append({"bits": bits, "access": access, "reset": reset})
        else:
            fields.append((mask, access, reset))
        if otp:
            otp_bits |= mask
        elif "X" not in reset.upper():
            known |= mask
            literal |= (int(reset.rstrip("h"), 16) << low) & mask
    require(all_bits == 0xFFFF and reserved == hex16(register["reservedMask"])
            and reserved_rows == register["reservedRows"],
            f"{name}: reserved partition differs from TI table")
    known_mask, value = hex16(register["resetKnownMask"]), hex16(register["resetValue"])
    known &= ~VARIABLE_RESET_BITS.get(name, 0)
    require(known_mask == known and value == literal & known,
            f"{name}: reset knowledge contradicts detailed table")
    covered = reserved
    for field in register["fields"]:
        mask = hex16(field["mask"])
        require(mask and not covered & mask, f"{name}: overlapping field or reserved bit")
        require(field.get("defaultFromOtp", False) is bool(otp_bits & mask),
                f"{field['id']}: OTP annotation differs from TI table")
        rows = [r for r in fields if mask & r[0]]
        require(sum(r[0] for r in rows) == mask
                and all(r[1:] == (field["access"], field["reset"]) for r in rows),
                f"{name}: {field['id']} field range/access/reset")
        covered |= mask
    require(covered == 0xFFFF, f"{name}: missing field")


def check_recipe(procedures, source, register_map):
    section = source.split("8.3.3.1 EEE Overview\n", 1)[1].split("8.3.3.2 EEE Negotiation", 1)[0]
    rows = re.findall(r"^(1F|7)h\n([0-9A-F]{4})h\n([0-9A-F]{4})h$", section, re.M)
    recipe = procedures["eeeRecipe"]
    require(len(rows) == len(recipe) == 27, "EEE recipe must contain all 27 ordered rows")
    for index, ((device, printed_address, value), row) in enumerate(zip(rows, recipe), 1):
        address = int(printed_address, 16)
        if device == "7":
            require(address == 0x203C, "unexpected printed MMD7 tuple")
            space, devad, wire = "MMD", 7, 0x003C
        elif address < 32:
            space, devad, wire = "C22", None, address
        else:
            space, devad, wire = "MMD", 31, address
            require(wire in register_map, f"EEE row {index}: address missing from inventory")
        actual_devad = None if row["devad"] is None else int(row["devad"], 16)
        require(row["printed"] == f"{device}:{printed_address}"
                and (row["space"], actual_devad, hex16(row["address"]), hex16(row["value"])) ==
                (space, devad, wire, int(value, 16)), f"EEE row {index}: order, wire address or data")


def check_assignments(original, procedures, registers):
    groups = {}
    baseline = {}
    for group in ("procedures", "features", "gates", "timings", "writeActions"):
        records = indexed(procedures[group], group)
        groups[group] = records
        baseline[group] = indexed(original[group], group)
        expected = set(baseline[group])
        if group == "writeActions":
            expected.update(r["id"] + "_eee" for r in registers.values()
                            if "originalRegister" in r and r["originalRegister"] is None)
        require(expected == records.keys(), f"incomplete or extra {group} comparison")
        for name, row in records.items():
            require(row["owner"] in (58, 59) and row["source"] and row["disposition"],
                    f"{group}.{name}: missing owner, source or disposition")
    for name, feature in groups["features"].items():
        base = baseline["features"][name]
        require(feature["owner"] == (58 if name in CORE_FEATURES else 59)
                and sorted(feature["procedures"]) == sorted(base["procedures"])
                and sorted(feature["gates"]) == sorted(base["gates"]), f"{name}: feature comparison")
    for name, proc in groups["procedures"].items():
        base = baseline["procedures"][name]
        require(proc["feature"] == base["feature"] and proc["test"].strip()
                and proc["owner"] == groups["features"][base["feature"]]["owner"],
                f"{name}: wrong feature, owner or regression requirement")
        owned_rows = indexed(proc.get("ownedMasks", base["ownedMasks"]), name, "register")
        require({reg: r["mask"] for reg, r in owned_rows.items()} == procedure_masks(base),
                f"{name}: incorrect procedure masks")
        for owned in owned_rows.values():
            reg = registers[owned["register"]]
            writable = sum(hex16(f["mask"]) for f in reg["fields"] if "W" in f["access"])
            require(not hex16(owned["mask"]) & ~writable,
                    f"{proc['id']}: owned mask includes reserved or read-only bits in {reg['id']}")
    for name, gate in groups["gates"].items():
        require(gate["owner"] == (58 if name in CORE_GATES else 59), f"{name}: wrong gate owner")
    for name, row in groups["timings"].items():
        base = baseline["timings"][name]
        owner = 59 if name.startswith("eee_") or name == "active_sleep_nlp_interval" else 58
        require(row["owner"] == owner and all(row[k] == base[k] for k in ("value", "unit", "qualifier")),
                f"{name}: timing owner, value, unit or qualifier")
    # Each vendor word in the ordered recipe needs an exact action. Direct
    # reset/restart and the IEEE advertisement have independent typed owners.
    recipe = procedures["eeeRecipe"]
    procedure = groups["procedures"]["eee_initialize"]
    write_frames = sum(1 if row["space"] == "C22" else 4 for row in recipe)
    # Capability and advertisement each require a fresh four-frame MMD read.
    require(procedure.get("frameFloor") == {
        "recipeWrites": write_frames,
        "withCapabilityAndAdvertisementReads": write_frames + 8,
    } and procedure.get("timeout", "").strip(), "EEE frame floor or timeout override")
    expected_rows = [i for i, r in enumerate(recipe, 1) if r["devad"] == "0x1F"]
    claimed = []
    for action in groups["writeActions"].values():
        name = action["id"]
        require(action["owner"] == 59, f"{name}: wrong write action owner")
        if not name.endswith("_eee"):
            base = baseline["writeActions"][name]
            require("rows" not in action and all(action[k] == base[k] for k in ("mask", "value"))
                    and action.get("neutral", base["neutral"]) == base["neutral"],
                    f"{name}: changed inherited exceptional action")
            continue
        require("rows" in action, f"{name}: missing ordered EEE positions")
        register = registers[name.removesuffix("_eee")]
        positions = action["rows"]
        if register.get("status") == "ORIGINAL_ONLY":
            require(positions == [] and action["value"] is None, "original-only EEE action")
            continue
        require(positions and action["mask"] == "0xFFFF", "unassigned EEE action")
        for pos in positions:
            require(type(pos) is int and 1 <= pos <= len(recipe), "invalid EEE action position")
            row = recipe[pos - 1]
            require((row["address"], row["value"]) == (register["address"], action["value"]),
                    f"EEE row {pos}: action disagrees with recipe")
            claimed.append(pos)
    require(sorted(claimed) == expected_rows, "incomplete or overlapping EEE row ownership")


def validate(original, comparison, procedures):
    for data in (comparison, procedures):
        require(data["variants"] == ["DP83826AI", "DP83826AE"], "missing exact A declaration")
    source = SOURCE.read_text(encoding="utf-8")
    original_regs = indexed(original["registers"], "original registers")
    detailed = indexed(comparison["registers"], "A registers")
    expected = {name for name, reg in original_regs.items() if reg["access"] != "RECIPE_ONLY"}
    require(expected == detailed.keys(), "missing or extra detailed register")
    headers = list(HEADER.finditer(source))
    require(len(headers) == len(detailed) == 79, "expected 79 SNLS783A detailed tables")
    by_address = indexed(comparison["registers"], "register addresses", "address")
    original_fields = set()
    mapped_fields = []
    features = indexed(procedures["features"], "features")
    for i, heading in enumerate(headers):
        reg = by_address[f"0x{int(heading[2], 16):04X}"]
        base = original_regs[reg["id"]]
        for key in ("address", "space", "devad", "aliases"):
            require(reg[key] == base[key], f"{reg['id']}: {key} differs from original address route")
        end = headers[i + 1].start() if i + 1 < len(headers) else source.index(
            "9 Application and Implementation", heading.end())
        check_register(reg, heading, source[heading.end():end])
        base_fields = indexed(base["fields"], base["id"])
        original_fields.update(base_fields)
        added = ADDED_FIELDS.get(reg["id"], {})
        require({f["id"] for f in reg["fields"] if f["originalField"] is None} == added.keys(),
                f"{reg['id']}: unexpected added field")
        for f in indexed(reg["fields"], reg["id"]).values():
            name = f["id"]
            if f["originalField"] is None:
                expected_mask, owner = added[name]
                require(expected_mask & hex16(base["reservedMask"]) == expected_mask,
                        f"{name}: added field was not reserved in original")
            else:
                require(f["originalField"] == f["id"] and f["id"] in base_fields,
                        f"{f['id']}: unknown original field")
                inherited = base_fields[name]
                expected_mask = hex16(inherited["mask"])
                if reg["id"] == "LEDCFG":
                    # SNLS783A table 8-86 relocates all three source nibbles.
                    expected_mask >>= 4
                require(inherited["owner"] in features, f"{name}: missing owning feature")
                owner = FIELD_OWNER_EXCEPTIONS.get(name, features[inherited["owner"]]["owner"])
                mapped_fields.append(f["originalField"])
            require(hex16(f["mask"]) == expected_mask, f"{name}: wrong semantic field mask")
            require(f["owner"] == owner and owner in (58, 59), f"{name}: wrong field owner")
    removed = indexed(comparison["removedFields"], "removed fields", "originalField")
    require(removed.keys() == {"PHYCR.led_config", "IO_CFG1.clock_off"}
            and set(mapped_fields) == original_fields - removed.keys()
            and len(mapped_fields) == len(set(mapped_fields)), "incomplete original field comparison")
    for name, f in removed.items():
        reg = detailed[name.split(".")[0]]
        base_field = next(row for row in original_regs[reg["id"]]["fields"] if row["id"] == name)
        mask = hex16(base_field["mask"])
        require(f["register"] == reg["id"] and f["table"] == reg["table"] and f["owner"] == 59
                and hex16(f["mask"]) == mask and mask & hex16(reg["reservedMask"]) == mask,
                f"{name}: removed field register, table, mask or owner")
    original_printed = indexed(original["printedAddresses"], "original printed addresses")
    printed = indexed(comparison["printedAddresses"], "A printed addresses")
    require(printed.keys() == original_printed.keys(), "incomplete printed-address comparison")
    for name, row in printed.items():
        require(all(row[key] == original_printed[name][key] for key in
                    ("devad", "printedAddress", "wireAddress", "readRoute", "status")) and row["owner"] == 59,
                f"{name}: changed printed/wire address or acquisition route")
    debug = indexed(comparison["debugAddresses"], "debug observations", "address")
    require(debug.keys() == {"0x0225"} and debug["0x0225"]["status"] == "UNRESOLVED"
            and debug["0x0225"]["owner"] == 59, "unresolved MSE address")
    recipe_regs = indexed(comparison["recipeRegisters"], "recipe registers", "address")
    original_recipes = original_regs.keys() - expected
    require({r["originalRegister"] for r in recipe_regs.values() if r["originalRegister"]} ==
            original_recipes, "incomplete original recipe comparison")
    for reg in recipe_regs.values():
        name = reg["id"]
        status = "ORIGINAL_ONLY" if name == "EEE_04D3" else "PROCEDURE_ONLY"
        require(reg["owner"] == 59 and reg["space"] == "MMD" and reg["devad"] == "0x1F"
                and reg["status"] == status, f"{name}: recipe-only address route or status")
        require(reg["resetKnownMask"] is None and reg["resetValue"] is None,
                f"{reg['id']}: undocumented recipe register reset")
        if reg["originalRegister"]:
            require(reg["originalRegister"] == name and reg["address"] == original_regs[name]["address"],
                    "recipe register address mismatch")
        else:
            require(name == "EEE_" + reg["address"][2:] and name not in original_regs,
                    f"{name}: incorrect A-only EEE register")
    active_addresses = {hex16(r["address"]) for r in [*detailed.values(), *recipe_regs.values()]
                        if r.get("status") != "ORIGINAL_ONLY"}
    check_recipe(procedures, source, active_addresses)
    check_assignments(original, procedures, {**detailed, **indexed(comparison["recipeRegisters"], "recipe IDs")})
