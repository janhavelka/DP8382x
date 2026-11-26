"""DP83826 opt-in control procedures and evidence for the compiled fixture."""

from dataclasses import dataclass, replace

if __package__:
    from .dp8382x_hil_campaign import (
        I32, WORD, AssertionFailed, Intervention, Profile, bits, clean, feature_fields, matches, payload, require,
    )
    from .dp8382x_hil_protocol import TERMINAL_STATUS, U64_MAX
else:
    from dp8382x_hil_campaign import (
        I32, WORD, AssertionFailed, Intervention, Profile, bits, clean, feature_fields, matches, payload, require,
    )
    from dp8382x_hil_protocol import TERMINAL_STATUS, U64_MAX


CASES = {
    "irq": ("wired IRQ, controlled event source and edge instrument", True),
    "pins": ("ENHANCED pin loads and probes with exclusive GPIO/clock ownership", True),
    "pins-basic": ("BASIC LED loads and probes with released pin consumers", True),
    "interface-mii": ("wired 25 MHz MII, independent management reference and timing analyzer", True),
    "interface-rmii25": ("wired RMII leader and scope for the 25 MHz input and 50 MHz reference", True),
    "interface-rmii50": ("qualified 50 MHz RMII reference, frame source and timing analyzer", True),
    "repeater-leader": ("leader owner in crossed two-PHY RMII fixture with one driven reference", True),
    "repeater-follower": ("follower owner in crossed two-PHY RMII fixture with a qualified shared reference", True),
    "power-ieee": ("authorized IEEE power transition and independent console/reference, current and clock probes", True),
    "power-sleep": ("controlled quiet/active copper peer, current and clock probes", True),
    "power-pll": ("independent retained management clock and console, instrumented PLL clock/current", True),
    "power-deep": ("source-qualified deep-power entry and exit procedure", False),
    "eee": ("100/full AN EEE partner and line/partner LPI observation", True),
    "lpi": ("drained TX, 100/full AN EEE partner and line/partner LPI observation", True),
    "wol-magic": ("identified Magic Packet source, nonmatching control, wake path and indication instrument", True),
    "wol-secure": ("isolated synthetic Secure-ON model or separately controlled private credential fixture", True),
    "wol-pulse": ("wake source and scoped, loaded pulse route", True),
    "quality": ("MII, calibrated symbol and energy faults, 10/100 partner and timing analyzer", True),
    "mse": ("100/full AN peer and independently calibrated quality stimulus and measurement", False),
    "analog": ("calibrated differential MDI load and amplitude, impedance and slew instruments", True),
}
P25_CASES = {"analog", "quality", "pins", "power-deep", "eee", "lpi", "power-ieee", "power-sleep", "power-pll", "mse", "irq", "wol-magic", "wol-pulse", "wol-secure"}
MII_CASES = {"interface-mii", "quality"}
RMII_CASES = {"pins", "interface-rmii25", "interface-rmii50", "repeater-leader", "repeater-follower"}
MODES = {"pins": "enhanced", "pins-basic": "basic", "interface-mii": "enhanced"}
PEERS = dict.fromkeys(("eee", "lpi", "mse", "irq"), "an-100-full")
BLOCKED_CASES = {"power-deep"}

IRQ_BASE = "irq configure 0 0 polarity intr 0"
EEE_BASE = "eee configure off 0 0"
# Synthetic packet-test octets. Never accept a real address or password as runner input.
MAC = "11:22:33:44:55:66"
WOL_BASE = f"wol configure off status 1 level 0 {MAC}"


def word_fields(names):
    return dict.fromkeys(names.split(), WORD)


STATUS = tuple(TERMINAL_STATUS)
OP = {
    "core_status": STATUS, "core_detail": I32, "effective_reset": ("none", "hard"),
    **bits("mutation_completed mutation_possible cleanup_owed temporary persistent_restored reset_accepted reset_complete"),
}
DIRECT = {
    "attempted": (0, 512), "completed": (0, 512), "transactions": (0, 512),
    "batch_primary": STATUS, "batch_detail": I32, "release_status": STATUS, "release_detail": I32,
    **bits("mutation_attempted mutation_possible verified"),
}
IRQ = {
    **bits("valid stale test_active physcr_valid misr1_valid misr2_valid rxfs_valid"),
    "trust": (0, 3), "selector": (0, 2), **word_fields("physcr misr1 misr2 rxfs"),
}
IRQ_SERVICE = {
    **bits("valid stale more_pending fcscr_requested fcscr_valid fcscr_lower_bound recr_requested recr_valid"),
    **word_fields("physts misr1 misr2 fcscr recr"), "shared_source": (0, 2), "last_status": STATUS,
}
POWER = {
    **bits("valid stale requested_down indicated_down requested_sleep requested_pll_off bmcr_valid physcr_valid biscr_valid"),
    "trust": (0, 3), "power_state": (0, 4), **word_fields("bmcr physcr biscr"),
}
EEE = {
    **bits("valid stale desired_valid enabled capability_valid advertised_valid partner_valid eee_status_valid "
           "wake_errors_valid control_valid negotiated_valid negotiated"),
    "trust": (0, 3), **word_fields("capability advertised partner eee_status wake_errors control"),
}
LPI = {
    "lpi": (0, 2), **bits("readback_valid effect_completed effect_possible"), "control": WORD,
    "batch_primary": STATUS, "batch_release": STATUS, "primary_detail": I32, "release_detail": I32,
    "attempted": (0, 9), "completed": (0, 9), "transactions": (0, 2),
}
WOL = {
    **bits("desired_valid stale control_valid enabled secure level crc magic rxfs_valid misr2_valid"),
    "trust": (0, 3), "route": (0, 5), "pulse": (0, 3), "match_word_mask": (0, 63),
    **word_fields("control rxfs misr2"),
}
STABLE = {**bits("desired stale"), "trust": (0, 3), "valid_mask": (0, 1023)}
PINS = {**STABLE, **word_fields("ledcr phycr mledcr ledcfg ledcfg2 io_cfg1 gpio0 gpio1 gpio2 gpio3")}
INTERFACE = {**STABLE, **bits("mode_valid"), **word_fields("cr1 cr2 physcr gencfg io_cfg1 rcsr bmcr clk_led_status")}
QUALITY = {**STABLE, **bits("fld_bound_applicable"), **word_fields("cr3 fld_cfg1 fld_cfg2 physcr cr2 btscr config10 cr1")}
ANALOG = {**STABLE, **word_fields("vod1 vod2 vod3 ioctrl io_cfg1")}
MSE = {**bits("valid stale observed_valid context_valid context_stale context_up"),
       "raw_mse": WORD, "observed_us": (0, U64_MAX), "last_status": STATUS}


@dataclass(frozen=True)
class Setting:
    """Typed profile and the expected stable readback from a qualified model."""
    command: str
    fields: dict


@dataclass(frozen=True)
class Control:
    """One independently replayable persistent control fixture."""
    schema: dict
    status: str
    baseline: Setting
    target: Setting
    form: str = "operation"
    live: str = ""


def pins(arguments, validity, **values):
    return Setting("controls pins " + arguments, dict(desired=1, trust=1, stale=0, valid_mask=validity, **values))


def interface(arguments, validity=127, **values):
    return Setting("controls interface " + arguments,
                   dict(desired=1, trust=1, stale=0, valid_mask=validity, mode_valid=1, **values))


def quality(arguments, **values):
    return Setting("controls quality " + arguments,
                   dict(desired=1, trust=1, stale=0, valid_mask=255, fld_bound_applicable=0, **values))


def analog(arguments, **values):
    return Setting("controls analog " + arguments, dict(desired=1, trust=1, stale=0, valid_mask=31, **values))


def power(request, sleep=0, pll=0):
    # The model holds BISCR normal. Sleep policy bits prove programming only.
    down = int(request == "down")
    return Setting(f"power configure {request} {sleep} {pll}",
                   dict(valid=1, trust=1, stale=0, power_state=3 if down else 1,
                        requested_down=down, indicated_down=down, requested_sleep=sleep, requested_pll_off=pll,
                        bmcr_valid=1, bmcr=0x1000 | down << 11, physcr_valid=1, physcr=sleep * 0x6000 | pll << 15,
                        biscr_valid=1, biscr=0x0100))


RMII25_BASE = interface("rmii25 0 0 1518 100 0 keep off", cr1=32, cr2=0, physcr=0, gencfg=0,
                        io_cfg1=0, rcsr=96, bmcr=4096, clk_led_status=0)
RMII50_BASE = interface("rmii50 0 0 1518 50 0 keep off", cr1=32, cr2=0, physcr=0, gencfg=0,
                        io_cfg1=0, rcsr=224, bmcr=4096, clk_led_status=0)
CONTROLS = {
    "pins": Control(PINS, "controls pins-status",
                    pins("led,0,low,normal led,0,low,normal led,0,low,normal led,0,keep,normal 0 enabled off keep",
                         1023, ledcr=0, phycr=16385, mledcr=1, ledcfg=5, ledcfg2=0,
                         io_cfg1=64, gpio0=0, gpio1=0, gpio2=8, gpio3=0),
                    pins("led,8,high,high led,2,high,low high led,6,keep,normal 1 bypass on keep",
                         1023, ledcr=658, phycr=16513, mledcr=577, ledcfg=24613, ledcfg2=5,
                         io_cfg1=64, gpio0=0, gpio1=0, gpio2=15, gpio3=0), "terminal"),
    "pins-basic": Control(PINS, "controls pins-status",
                          pins("led,0,keep,normal led,0,keep,normal keep keep 0 enabled keep keep",
                               223, ledcr=0, phycr=16385, mledcr=1, ledcfg=22021, ledcfg2=0,
                               io_cfg1=0, gpio0=0, gpio1=0, gpio2=0, gpio3=0),
                          pins("led,8,keep,high led,2,keep,low keep keep 1 bypass keep keep",
                               223, ledcr=530, phycr=16513, mledcr=65, ledcfg=22053, ledcfg2=1,
                               io_cfg1=0, gpio0=0, gpio1=0, gpio2=0, gpio3=0), "terminal"),
    "interface-mii": Control(INTERFACE, "controls interface-status",
                             interface("mii 0 0 1518 50 0 clkout25 off", 255, cr1=32, cr2=0, physcr=0,
                                       gencfg=0, io_cfg1=0, rcsr=65, bmcr=4096, clk_led_status=0),
                             interface("mii 0 0 1518 50 15 led1 on", 255, cr1=34, cr2=2, physcr=16,
                                       gencfg=8, io_cfg1=64, rcsr=65, bmcr=5120, clk_led_status=1), "terminal"),
    "interface-rmii25": Control(INTERFACE, "controls interface-status", RMII25_BASE,
                                interface("rmii25 6 2 1518 100 10 keep off", cr1=32, cr2=2, physcr=0,
                                          gencfg=8, io_cfg1=256, rcsr=114, bmcr=4096, clk_led_status=0), "terminal"),
    "interface-rmii50": Control(INTERFACE, "controls interface-status", RMII50_BASE,
                                interface("rmii50 7 3 1518 50 10 keep off", cr1=32, cr2=2, physcr=0,
                                          gencfg=8, io_cfg1=256, rcsr=499, bmcr=4096, clk_led_status=0), "terminal"),
    "repeater-leader": Control(INTERFACE, "controls interface-status", RMII25_BASE,
                               interface("rmii25 12 0 1518 100 2 keep off", cr1=32, cr2=2, physcr=0,
                                         gencfg=0, io_cfg1=256, rcsr=96, bmcr=4096, clk_led_status=0), "terminal"),
    "repeater-follower": Control(INTERFACE, "controls interface-status", RMII50_BASE,
                                 interface("rmii50 12 0 1518 50 2 keep off", cr1=32, cr2=2, physcr=0,
                                           gencfg=0, io_cfg1=256, rcsr=224, bmcr=4096, clk_led_status=0), "terminal"),
    "power-ieee": Control(POWER, "power status", power("normal"), power("down"), live="power read"),
    "power-sleep": Control(POWER, "power status", power("normal"), power("normal", sleep=1), live="power read"),
    "power-pll": Control(POWER, "power status", power("normal"), power("down", pll=1), live="power read"),
    "quality": Control(QUALITY, "controls quality-status",
                       quality("0x404,2,5,9 0,0,0 1,4,0,0,0 off", cr3=1124, fld_cfg1=2135,
                               fld_cfg2=8841, physcr=0, cr2=0, btscr=10240, config10=14744, cr1=32),
                       quality("0x405,3,6,10 1,3,1 1,8,1,1,1 on", cr3=1125, fld_cfg1=3175,
                               fld_cfg2=8842, physcr=2816, cr2=4, btscr=12417, config10=31128, cr1=160)),
    "analog": Control(ANALOG, "controls analog-status",
                      analog("0x2f,0x2f,8,0x18,0 4,4,1,1 fast slow keep", vod1=11200,
                             vod2=62984, vod3=19008, ioctrl=16, io_cfg1=0),
                      analog("0x28,0x2f,0x18,8,0 4,4,0,1 slow fast keep", vod1=10752,
                             vod2=61976, vod3=16960, ioctrl=17, io_cfg1=16384)),
}

P25_POWER = {**POWER, "power_state": (0, 5), "deep_word": WORD,
             **bits("deep_valid deep_attempt deep_complete deep_possible")}


def p25_control(name, role):
    """SNLS638C field expectations for the two declared RMII roles of the offline fixture."""
    if name in ("power-ieee", "power-pll", "power-sleep", "power-deep"):
        base = power("normal")
        target = power("down") if name == "power-deep" else CONTROLS[name].target
        if name == "power-deep":
            target = Setting("power configure deep 0 0", {**target.fields, "power_state": 5})
        def state(setting, deep):
            return replace(setting, fields={**setting.fields, "deep_valid": 1, "deep_word": 4 if deep else 0,
                                            "deep_attempt": int(deep), "deep_complete": int(deep), "deep_possible": int(deep)})
        return Control(P25_POWER, "power status", state(base, False), state(target, name == "power-deep"), live="power read")
    if name == "pins":
        if role == "leader":
            # Keep the generated reference pin under the clock owner for this
            # campaign. Only LED0 and blink are acquired, applied and restored.
            fields = dict(phycr=0x4001, ledcfg=0, ledcfg2=0, io_cfg1=0, gpio0=0, gpio1=0, gpio2=0, gpio3=0)
            base = pins("led,0,low,normal keep keep keep 0 keep keep keep", 7, ledcr=0, mledcr=1, **fields)
            target = pins("led,8,high,high keep keep keep 1 keep keep keep", 7, ledcr=0x292, mledcr=0x41, **fields)
            return Control(PINS, "controls pins-status", base, target, "terminal")
        # LED0 uses LEDCR/LED_CFG1, LED2 uses LED_CFG2[6:4]/LED_CFG1[7:4].
        # Preserve the model's reserved nibble, PHY address and unowned sources.
        fields = dict(phycr=0x4001, io_cfg1=0, gpio0=0, gpio1=0, gpio2=9, gpio3=0)
        base = pins("led,0,low,normal keep led,0,low,normal keep 0 keep keep keep", 0x11F,
                    ledcr=0, mledcr=1, ledcfg=0x5605, ledcfg2=0, **fields)
        target = pins("led,8,high,high keep led,14,high,high keep 1 keep keep keep", 0x11F,
                      ledcr=0x292, mledcr=0x41, ledcfg=0x56E5, ledcfg2=0x70, **fields)
        return Control(PINS, "controls pins-status", base, target, "terminal")
    if name == "quality":
        common = dict(desired=1, trust=1, stale=0, valid_mask=0xB9, fld_bound_applicable=0,
                      fld_cfg1=0, fld_cfg2=0, physcr=0, config10=0, cr1=32)
        base = Setting("controls quality 0 0,0,0 0,0,0,0,0 keep", dict(cr3=0x60, cr2=0, btscr=0, **common))
        target = Setting("controls quality 0x405 0,0,1 1,8,1,1,0 keep", dict(cr3=0x465, cr2=4, btscr=0x3081, **common))
        return Control(QUALITY, "controls quality-status", base, target)
    if name == "analog":
        # Table 8-2 coarse words, 0302[15:14] for 50/25 ohm. Table 8-3
        # authorizes only a write to 030E, so bit 2 in valid_mask stays clear.
        common = dict(desired=1, trust=1, stale=0, valid_mask=0x13, vod3=0, ioctrl=0)
        base = Setting("controls analog vod,8,0 keep keep 50ohm keep", dict(vod1=0x0C00, vod2=16, io_cfg1=0, **common))
        target = Setting("controls analog vod,9,-1 keep keep 25ohm keep", dict(vod1=0x0BC0, vod2=17, io_cfg1=0x4000, **common))
        return Control(ANALOG, "controls analog-status", base, target)
    return None


def a_control(name, mode):
    """Independently expected A register words for the simulated entry and target profiles."""
    physcr = 0x010B if mode == "basic" else 0x0108
    if name.startswith("power-") and name in CONTROLS:
        spec = CONTROLS[name]
        def setting(profile):
            return replace(profile, fields={**profile.fields, "physcr": physcr | profile.fields["physcr"]})
        return replace(spec, baseline=setting(spec.baseline), target=setting(spec.target))
    if name == "analog":
        return CONTROLS[name]
    if name == "pins-basic":
        base = pins("led,0,keep,normal led,0,keep,normal keep keep 0 enabled keep keep",
                    223, ledcr=0, phycr=0x4001, mledcr=1, ledcfg=0x5560, ledcfg2=0,
                    io_cfg1=0, gpio0=8, gpio1=8, gpio2=0, gpio3=0)
        target = pins("led,8,keep,high led,2,keep,low keep keep 1 bypass keep keep",
                      223, ledcr=530, phycr=0x4081, mledcr=65, ledcfg=0x5562, ledcfg2=1,
                      io_cfg1=0, gpio0=8, gpio1=8, gpio2=0, gpio3=0)
        return Control(PINS, "controls pins-status", base, target, "terminal")
    if name == "pins":
        # Pin 31 remains the POR clock output. Pin 29 uses its independent LED3 force bank.
        base = pins("led,0,low,normal keep led,0,low,normal led,0,low,normal 0 enabled off keep",
                    863, ledcr=0, phycr=0x4001, mledcr=1, ledcfg=0x5005, ledcfg2=0,
                    io_cfg1=0, gpio0=8, gpio1=0, gpio2=8, gpio3=8)
        target = pins("led,8,high,high keep led,4,low,normal led,10,high,high 1 bypass on keep",
                      863, ledcr=0x292, phycr=0x4081, mledcr=0x241, ledcfg=0x5A45, ledcfg2=0x700,
                      io_cfg1=0, gpio0=8, gpio1=0, gpio2=8, gpio3=8)
        return Control(PINS, "controls pins-status", base, target, "terminal")
    if name == "quality":
        # SNLS783A tables 8-29/33/36/45. Keep CR3[6:5] from entry MDIX.
        # The profiles exercise FIFO codes 0 and 3;
        # both own 10M squelch/preamble and preserve the other mode bits.
        base = quality("0x405 0,0,0 1,4,0,0,0 off", cr3=0x0465, fld_cfg1=0x0417, fld_cfg2=0x0288,
                       physcr=physcr & ~0x0B00, cr2=0x0102, btscr=0x2800, config10=0x3998, cr1=32)
        target = quality("0x40d 1,3,1 1,8,1,1,1 on", cr3=0x046D, fld_cfg1=0x0417, fld_cfg2=0x0288,
                         physcr=physcr | 0x0B00, cr2=0x0106, btscr=0x3081, config10=0x7998, cr1=160)
        return Control(QUALITY, "controls quality-status",
                       replace(base, fields={**base.fields, "fld_bound_applicable": 1}),
                       replace(target, fields={**target.fields, "fld_bound_applicable": 1}))
    if name == "interface-mii":
        # The ENHANCED case verifies the independent pin 30 PLL mux and pin 31 sample.
        base = interface("mii 0 0 1518 50 16 pll off", 495, cr1=32, cr2=0x0100, physcr=0x0108, gencfg=0,
                         io_cfg1=0, rcsr=65, bmcr=4096, clk_led_status=2, pin30_mux=0x29)
        target = interface("mii 0 0 1518 50 31 pll on", 495, cr1=34, cr2=0x0102, physcr=0x0118, gencfg=8,
                           io_cfg1=0, rcsr=65, bmcr=5120, clk_led_status=2, pin30_mux=0x29)
        return Control({**INTERFACE, "pin30_mux": WORD}, "controls interface-status", base, target, "terminal")
    if name in ("interface-rmii25", "interface-rmii50", "repeater-leader", "repeater-follower"):
        spec = CONTROLS[name]
        base = replace(spec.baseline, fields={**spec.baseline.fields, "physcr": physcr, "cr2": 0x0100})
        target = replace(spec.target, fields={**spec.target.fields, "physcr": physcr, "cr2": 0x0102})
        return replace(spec, baseline=base, target=target)
    return None


BASELINES = {case_id: (spec.baseline.command,) for case_id, spec in CONTROLS.items()}
BASELINES.update(irq=(IRQ_BASE,), eee=(EEE_BASE,), lpi=(EEE_BASE,),
                 pins=("controls interface rmii50 0 0 1518 50 0 led1 off", CONTROLS["pins"].baseline.command))
for _case_id in ("wol-magic", "wol-secure", "wol-pulse"):
    BASELINES[_case_id] = (WOL_BASE, "wol acknowledge")


def evidence(row, expected, message, verdict="FAIL"):
    require(matches(row.fields, **expected), row, message, verdict)


def inspect(c, text, schema, expected, *, cleanup=False, verdict="FAIL"):
    row = yield from c.command(text, cleanup=cleanup)
    # Failed reads may omit payloads or report an older retained prefix.
    require(clean(row), row, f"{text} did not complete cleanly", verdict)
    feature_fields(row, schema, verdict)
    evidence(row, expected, f"{text} does not prove the required observed control state", verdict)
    return row


def effect_possible(row):
    """An admission can install desired intent before the first bus write."""
    if row.op or row.primary == "OK":
        return True
    fields = payload(row)
    if fields in ({}, {"gate": "board"}) and row.primary in (
            "BUSY", "INVALID_CONFIG", "INVALID_PARAM", "NOT_INITIALIZED", "UNSUPPORTED"):
        return False
    if row.cmd == "lpi.start" and fields:
        try:
            feature_fields(row, LPI)
        except AssertionFailed:
            return True
        return not matches(fields, lpi=0, effect_completed=0, effect_possible=0)
    return True


def issue_mutation(c, text, form):
    old = c.may_change
    c.may_change = True
    row = yield from c.command(text, form)
    c.may_change = old or effect_possible(row)
    return row


def operation(row, schema, expected, *, reset=False, temporary=False):
    require(clean(row), row, f"{row.cmd} did not complete cleanly")
    feature_fields(row, {**OP, **schema})
    evidence(row, dict(core_status="OK", core_detail=0, mutation_completed=1, mutation_possible=1,
                       cleanup_owed=int(temporary), temporary=int(temporary), persistent_restored=int(not temporary),
                       effective_reset="hard" if reset else "none", reset_accepted=int(reset),
                       reset_complete=int(reset), **expected),
             f"{row.cmd} lacks a verified effect, reset or control receipt")


def direct(row, schema, expected):
    require(clean(row), row, f"{row.cmd} did not complete cleanly")
    feature_fields(row, {**DIRECT, **schema})
    evidence(row, dict(batch_primary="OK", batch_detail=0, release_status="OK", release_detail=0,
                       mutation_attempted=1, mutation_possible=1, verified=1, **expected),
             f"{row.cmd} lacks verified effects or independent release")
    require(0 < int(row.fields["transactions"]) <= int(row.fields["completed"]) == int(row.fields["attempted"]),
            row, f"{row.cmd} has incomplete frame accounting")


def cleanup_error(c, error):
    c.cleanup_failure = ". ".join(filter(None, (c.cleanup_failure, str(error))))
    if c.restoration != "FAIL":
        c.restoration = error.verdict


def persistent_guard(c):
    """Prove the cleanup route and prevent changing persistent intent with temporary debt."""
    slot = yield from c.guard_feature_cleanup()
    evidence(slot, {"cleanup_owed": 0, "temporary": 0}, "earlier effects bar the typed persistent restore")


def clear_persistent_debt(c, slot, spec, saved):
    """Use a separately authorized no-reset route for known simulated intent."""
    require(c.spec.id in ("pins", "pins-basic", "quality", "analog") and c.declaration["reinitialize"],
            slot, "cleanup debt requires a qualified no-reset replay route")
    desired = yield from c.profile("desired", True)
    require(matches(desired.fields, valid=1, stale=0) and Profile.read(desired.fields) == saved,
            desired, "saved link intent was lost before replay")
    yield from inspect(c, spec.status, spec.schema, {"desired": 1}, cleanup=True)
    yield from c.reset("none", True)
    yield from observe(c, spec, spec.target, cleanup=True)
    yield from persistent_guard(c)


def apply(c, spec, setting, *, cleanup=False):
    if cleanup:
        row = yield from c.command(setting.command, spec.form, True)
    else:
        row = yield from issue_mutation(c, setting.command, spec.form)
    if spec.form == "operation":
        operation(row, spec.schema, setting.fields)
    else:
        direct(row, spec.schema, setting.fields)


def observe(c, spec, setting, *, cleanup=False, verdict="FAIL"):
    if spec.live:
        yield from inspect(c, spec.live, spec.schema, setting.fields, cleanup=cleanup, verdict=verdict)
    return (yield from inspect(c, spec.status, spec.schema, setting.fields, cleanup=cleanup, verdict=verdict))


def persistent_case(c, saved):
    """Verify an explicit entry baseline, apply one profile and prove its independent restoration."""
    spec = c.part_control or CONTROLS[c.spec.id]
    try:
        yield from observe(c, spec, spec.baseline, verdict="BLOCKED")
        yield from apply(c, spec, spec.target)
        yield from observe(c, spec, spec.target)
    except AssertionFailed as error:
        c.failed = error
    if not c.may_change:
        return
    c.restoration = "INCONCLUSIVE"
    try:
        slot = yield from c.guard_feature_cleanup()
        evidence(slot, {"temporary": 0}, "temporary stimulus bars persistent restore")
        if slot.fields["cleanup_owed"] == "1":
            yield from clear_persistent_debt(c, slot, spec, saved)
        if c.spec.id == "power-pll":
            # The ordinary NORMAL command lacks the clock-loss grant. Clear PLL
            # while retaining independently proved IEEE down, then exit IEEE down.
            current = yield from inspect(c, "power read", spec.schema,
                                         dict(stale=0, bmcr_valid=1, physcr_valid=1, biscr_valid=1), cleanup=True)
            if int(current.fields["physcr"]) & 0x8000:
                evidence(current, {"indicated_down": 1, "power_state": 3},
                         "cannot select PLL exit without current IEEE down")
                interim = (p25_control("power-ieee", c.expected["rmii_role"]).target if c.p25 else
                           a_control("power-ieee", c.declaration["mode"]).target if c.a_feature else power("down"))
                yield from apply(c, spec, interim, cleanup=True)
                yield from observe(c, spec, interim, cleanup=True)
        baseline = spec.baseline
        if c.p25 and c.spec.id == "power-deep":
            # Clearing DPDWN has its own effect receipt even after the final
            # NORMAL observation matches the explicitly installed entry state.
            baseline = replace(baseline, fields={**baseline.fields, "deep_attempt": 1, "deep_complete": 1, "deep_possible": 1})
        yield from apply(c, spec, baseline, cleanup=True)
        yield from observe(c, spec, baseline, cleanup=True)
    except AssertionFailed as error:
        cleanup_error(c, error)
    else:
        c.restoration = "PASS"


def inject(c, action):
    """Require a correlated model action receipt before reading synthetic evidence."""
    row = yield Intervention(action, False)
    require(row["outcome"] == "acknowledged", c.last, f"{action} did not complete", "INCONCLUSIVE")


def irq(c):
    """Own and stop interrupt stimulus, consuming one guarded modeled event sample."""
    output_enable = 0x0101 if c.a_feature else int(c.declaration["mode"] in ("enhanced", "none"))
    stable = dict(valid=1, trust=1, stale=0, physcr_valid=1, misr1_valid=1, misr2_valid=1, rxfs_valid=1)
    baseline = dict(stable, selector=1, test_active=0, physcr=8 | output_enable, misr1=0, misr2=0, rxfs=0)
    target = dict(stable, selector=2, test_active=0, physcr=10 | output_enable, misr1=32, misr2=2, rxfs=4096)
    active = dict(target, test_active=1, physcr=target["physcr"] | 4)
    test_owed = False
    try:
        yield from inspect(c, "irq status", IRQ, baseline, verdict="BLOCKED")
        row = yield from issue_mutation(c, "irq configure 32 2 wake intr 2", "operation")
        operation(row, IRQ, target)
        yield from inspect(c, "irq status", IRQ, target)
        row = yield from issue_mutation(c, "irq test-start", "operation")
        test_owed = bool(row.op) and not matches(row.fields, test_active=0, cleanup_owed=0, temporary=0)
        operation(row, IRQ, active, temporary=True)
        yield from inspect(c, "irq status", IRQ, active)
        yield from inject(c, "inject-events")
        sample = yield from c.command("irq service 2 5 5")
        if not clean(sample):
            # The aggregate may fail after destructive words were consumed.
            # Own the cached collector without another live read.
            c.failed = AssertionFailed("IRQ collector did not complete", sample)
            yield from c.command("cached events", cleanup=True)
            yield from c.command("irq cached-service", cleanup=True)
            raise c.failed
        feature_fields(sample, IRQ_SERVICE)
        evidence(sample, dict(valid=1, stale=0, more_pending=1, physts=21, misr1=0xA520, misr2=0x5A02,
                              shared_source=2, fcscr_requested=0, fcscr_valid=0, fcscr=0, fcscr_lower_bound=0,
                              recr_requested=1, recr_valid=1, recr=0, last_status="OK"),
                 "IRQ event attribution and guarded counter evidence differ")
        before = yield from c.transport()
        cached = yield from inspect(c, "irq cached-service", IRQ_SERVICE, payload(sample))
        after = yield from c.transport()
        require(payload(before) == payload(after), cached, "cached IRQ service consumed management frames")
    except AssertionFailed as error:
        c.failed = error
    if not c.may_change:
        return
    c.restoration = "INCONCLUSIVE"
    if test_owed:
        try:
            yield from c.guard_feature_cleanup()
            row = yield from c.command("irq test-stop", "operation", True)
            operation(row, IRQ, target)
            yield from inspect(c, "irq status", IRQ, target, cleanup=True)
        except AssertionFailed as error:
            cleanup_error(c, error)
    try:
        yield from persistent_guard(c)
        row = yield from c.command(IRQ_BASE, "operation", True)
        operation(row, IRQ, baseline)
        yield from inspect(c, "irq status", IRQ, baseline, cleanup=True)
    except AssertionFailed as error:
        cleanup_error(c, error)
    if not c.cleanup_failure:
        c.restoration = "PASS"


def link100(c, saved, cleanup=False):
    row = yield from c.link(saved, saved.modes, cleanup=cleanup)
    evidence(row, dict(observation="up", valid=1, stale=0, up=1, resolved=1, speed_mbps=100, duplex="full",
                       negotiation="complete", partner_valid=1, partner_modes=8, pause_valid=1),
             "100/full AN partner and resolved mode are required")


def eee_read(c, enabled, cleanup=False, verdict="FAIL"):
    expected = dict(valid=1, stale=0, trust=1, desired_valid=1, enabled=int(enabled),
                    capability_valid=1, capability=2, advertised_valid=1, advertised=2 if enabled else 0,
                    partner_valid=1, partner=2, eee_status_valid=1, eee_status=0, wake_errors_valid=1,
                    wake_errors=0, control_valid=1, control=(0xC2 if c.p25 else 139) if enabled else 128,
                    negotiated_valid=int(enabled), negotiated=int(enabled))
    row = yield from inspect(c, "eee read", EEE, expected, cleanup=cleanup, verdict=verdict)
    yield from inspect(c, "eee status", EEE, payload(row), cleanup=cleanup, verdict=verdict)


def eee_terminal(row, enabled, a_grade=False):
    # An EEE reset terminal carries a stale history prefix. A failed earlier
    # acquisition can legitimately change it, so prove ownership here and
    # require separate live words after the reset.
    restart = {"recipe_restart": "accepted" if enabled else "none"} if a_grade else {}
    schema = {**EEE, "recipe_restart": ("accepted", "uncertain", "attempted", "none")} if a_grade else EEE
    operation(row, schema, dict(**restart, stale=1, trust=1, desired_valid=1, enabled=int(enabled),
                             advertised_valid=1, advertised=2 if enabled else 0,
                             negotiated_valid=0, negotiated=0), reset=True)


def lpi_result(row, enabled, p25=False):
    require(clean(row), row, f"{row.cmd} did not complete cleanly")
    schema = {**LPI, "min_normal_ifg_us": (36, 36)} if p25 and not enabled else LPI
    feature_fields(row, schema)
    evidence(row, dict(lpi=int(enabled), readback_valid=1, control=(0xC2 if p25 else 139) | int(enabled) << 12,
                       batch_primary="OK", primary_detail=0, batch_release="OK", release_detail=0,
                       attempted=9, completed=9, transactions=2, effect_completed=1, effect_possible=1),
             f"{row.cmd} did not prove the forced LPI request or its independent clear")


def eee(c, saved):
    """Qualify fresh AN/EEE evidence, and independently clear any forced LPI request."""
    lpi_owed = False
    try:
        yield from link100(c, saved)
        yield from eee_read(c, False, verdict="BLOCKED")
        row = yield from issue_mutation(c, "eee configure on 2 0", "operation")
        eee_terminal(row, True, c.a_feature)
        yield from link100(c, saved)
        yield from eee_read(c, True)
        if c.spec.id == "lpi":
            yield from inspect(c, "power read", P25_POWER if c.p25 else POWER, dict(valid=0, trust=0, stale=0, power_state=1, indicated_down=0,
                               requested_down=0, requested_sleep=0, requested_pll_off=0,
                               bmcr_valid=1, bmcr=4096, physcr_valid=1,
                               physcr=(0x010B if c.declaration["mode"] == "basic" else 0x0108) if c.a_feature else 0,
                               biscr_valid=1, biscr=256, **(dict(deep_valid=1, deep_word=0, deep_attempt=0,
                               deep_complete=0, deep_possible=0) if c.p25 else {})))
            yield from inspect(c, "lpi status", LPI, dict(lpi=0, readback_valid=0, control=0, attempted=0,
                               completed=0, transactions=0, effect_completed=0, effect_possible=0,
                               batch_primary="RESULT_NOT_AVAILABLE", primary_detail=0,
                               batch_release="RESULT_NOT_AVAILABLE", release_detail=0))
            row = yield from issue_mutation(c, "lpi start", "terminal")
            lpi_owed = effect_possible(row)
            lpi_result(row, True, c.p25)
            yield from inspect(c, "lpi status", LPI, payload(row))
    except AssertionFailed as error:
        c.failed = error
    if not c.may_change:
        return
    c.restoration = "INCONCLUSIVE"
    if lpi_owed:
        try:
            yield from c.guard_feature_cleanup()
            row = yield from c.command("lpi stop", cleanup=True)
            lpi_result(row, False, c.p25)
            yield from inspect(c, "lpi status", {**LPI, "min_normal_ifg_us": (36, 36)} if c.p25 else LPI, payload(row), cleanup=True)
        except AssertionFailed as error:
            cleanup_error(c, error)
    try:
        yield from persistent_guard(c)
        row = yield from c.command(EEE_BASE, "operation", True)
        eee_terminal(row, False, c.a_feature)
        yield from link100(c, saved, True)
        yield from eee_read(c, False, cleanup=True)
    except AssertionFailed as error:
        cleanup_error(c, error)
    if not c.cleanup_failure:
        c.restoration = "PASS"


def wake_view(enabled=False, secure=False, pulse=0, pending=False, event=False):
    control = 0x1000 | int(not pulse) << 8 | pulse << 9 | int(enabled) * 0x81 | int(secure) << 5 | int(pending) << 11
    return dict(desired_valid=1, trust=1, stale=0, route=0, control_valid=1, control=control, enabled=int(enabled),
                secure=int(secure), level=int(not pulse), pulse=pulse, crc=1, magic=int(enabled), match_word_mask=63,
                rxfs_valid=1, rxfs=0x1000 | int(event), misr2_valid=1, misr2=int(event) << 9)


def check_baseline(row, text, mode, a_grade=False):
    """Validate the simulator's pre-session write using its normal typed effect receipt."""
    for spec in CONTROLS.values():
        if text == spec.baseline.command:
            check = operation if spec.form == "operation" else direct
            check(row, spec.schema, spec.baseline.fields)
            return
    if text == BASELINES["pins"][0]:
        direct(row, INTERFACE, {**RMII50_BASE.fields, "valid_mask": 255, "io_cfg1": 64, "clk_led_status": 1})
    elif text == IRQ_BASE:
        operation(row, IRQ, dict(valid=1, stale=0, trust=1, selector=1, test_active=0,
                                 physcr_valid=1, physcr=(0x0108 | 1) if a_grade else 8 | int(mode in ("enhanced", "none")), misr1_valid=1, misr1=0,
                                 misr2_valid=1, misr2=0, rxfs_valid=1, rxfs=0))
    elif text == EEE_BASE:
        eee_terminal(row, False, a_grade)
    elif text in (WOL_BASE, "wol acknowledge"):
        operation(row, WOL, wake_view(pending=text == WOL_BASE))
    else:
        raise AssertionFailed("unrecognized compiled control setup", row)


def wol(c):
    """Check the synthetic detector and W0C receipt, with an independently sampled restore."""
    secure = c.spec.id == "wol-secure"
    pulse = 2 if c.spec.id == "wol-pulse" else 0
    password = " 2a:2b:2c:2d:2e:2f" if secure else ""
    command = f"wol configure {'secure' if secure else 'magic'} status 1 {'pulse' if pulse else 'level'} {pulse} {MAC}{password}"
    target = wake_view(True, secure, pulse)
    pending = False
    try:
        yield from inspect(c, "wol read", WOL, wake_view(), verdict="BLOCKED")
        row = yield from issue_mutation(c, command, "operation")
        operation(row, WOL, target)
        yield from inspect(c, "wol read", WOL, target)
        yield from inspect(c, "wol status", WOL, target)
        pending = not pulse
        yield from inject(c, "inject-wake")
        yield from inspect(c, "wol read", WOL, wake_view(True, secure, pulse, pending=pending, event=True))
        if pending:
            row = yield from c.command("wol acknowledge", "operation")
            operation(row, WOL, target)
        yield from inspect(c, "wol read", WOL, target)
        pending = False
    except AssertionFailed as error:
        c.failed = error
    if not c.may_change:
        return
    c.restoration = "INCONCLUSIVE"
    if pending:
        try:
            yield from persistent_guard(c)
            row = yield from c.command("wol read", cleanup=True)
            require(clean(row), row, "current wake policy unavailable during cleanup")
            feature_fields(row, WOL)
            if int(row.fields["control"]) & 0x0800:
                owned = wake_view(True, secure, pending=True)
                del owned["rxfs"], owned["misr2"]
                evidence(row, owned, "owned level wake cannot be proved for cleanup")
                require(int(row.fields["rxfs"]) & 0x1000, row, "wake selector is unavailable during cleanup")
                row = yield from c.command("wol acknowledge", "operation", True)
                operation(row, WOL, target)
        except AssertionFailed as error:
            cleanup_error(c, error)
    try:
        yield from persistent_guard(c)
        row = yield from c.command(WOL_BASE, "operation", True)
        operation(row, WOL, wake_view())
        yield from inspect(c, "wol read", WOL, wake_view(), cleanup=True)
        yield from inspect(c, "wol status", WOL, wake_view(), cleanup=True)
    except AssertionFailed as error:
        cleanup_error(c, error)
    if not c.cleanup_failure:
        c.restoration = "PASS"


def mse(c, saved):
    """Read one raw MSE word, correlated with an earlier resolved link observation."""
    yield from link100(c, saved)
    observed = int(c.last.fields["observed_us"])
    before = yield from c.transport()
    schema = {**MSE, "quality": (0, 3)} if c.p25 else MSE
    expected = {"quality": 3} if c.p25 else {}
    row = yield from inspect(c, "controls mse", schema, dict(**expected, valid=1, stale=0, raw_mse=0x1234,
                             observed_valid=1, context_valid=1, context_stale=0, context_up=1, last_status="OK"))
    require(max(observed, int(before.fields["time_us"])) < int(row.fields["observed_us"]) <=
            int(row.fields["time_us"]) + 1, row, "MSE sample timestamp does not follow current link evidence")
    yield from inspect(c, "controls mse-status", schema, payload(row))
    after = yield from c.transport()
    require(all(int(after.fields[name]) - int(before.fields[name]) == count
                for name, count in (("attempted", 4), ("completed", 4), ("batches", 1))),
            after, "MSE read plus passive cached copy did not consume exactly one MMD transaction")


def deep_gate(c):
    """Prove source rejection without a bus transfer."""
    before = yield from c.transport()
    row = yield from c.command("power configure deep 0 0", "operation")
    evidence(row, dict(primary="UNSUPPORTED", detail=4, cleanup="none"), "deep power source gate differs")
    require(not row.op and not payload(row), row, "deep power request returned effect evidence")
    after = yield from c.transport()
    require(payload(before) == payload(after), after, "deep power rejection changed bus counters")
    raise AssertionFailed("deep-power exit is source-gated", row, "BLOCKED")


def run(c, saved):
    """Run only the selected control case under the shared campaign owner and budgets."""
    case_id = c.spec.id
    if c.part_control or case_id in CONTROLS:
        yield from persistent_case(c, saved)
    elif case_id == "irq":
        yield from irq(c)
    elif case_id in ("eee", "lpi"):
        yield from eee(c, saved)
    elif case_id.startswith("wol-"):
        yield from wol(c)
    elif case_id == "power-deep":
        yield from deep_gate(c)
    else:
        yield from mse(c, saved)
