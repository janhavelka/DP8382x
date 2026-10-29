/* Research probe for the pinned SDK's C register types, not a PHY driver.
 * The SDK header is supplied externally and hash-checked by probe_sdk_mmd.py.
 * Explicit checks remain active with NDEBUG. No uninitialized C object is read.
 */
#include <stdint.h>
#include <stdio.h>
#include "eth_phy_802_3_regs.h"

int main(void)
{
    const uint32_t poison[] = {0U, 0xffffffffU, 0xa5a5a5a5U, 0x5a5a5a5aU};
    const uint32_t values[] = {0U, 1U, 0x8000U, 0xffffU};
    unsigned control_cases = 0;
    unsigned data_cases = 0;
    unsigned hazards = 0;
    for (unsigned p = 0; p < sizeof(poison) / sizeof(poison[0]); ++p) {
        for (unsigned devad = 0; devad < 32; ++devad) {
            for (unsigned function = 0; function < 2; ++function) {
                mmdctrl_reg_t partial;
                partial.val = poison[p];
                /* Same partial assignments as the audited SDK helpers, with
                 * defined initial poison instead of indeterminate storage. */
                partial.devaddr = devad;
                partial.function = function;
                const uint32_t expected = devad + function * 0x4000U;
                const uint32_t untouched = poison[p] & 0xffff3fe0U;
                if (partial.val != (expected | untouched)) {
                    fputs("FAIL: SDK C bitfield layout differs from audited model\n", stderr);
                    return 1;
                }
                hazards += partial.val != expected;
                /* Research vector assigned over poison, not production code. */
                uint32_t complete = poison[p];
                complete = ((uint32_t)function << 14) | (uint32_t)devad;
                if (complete != expected || (complete & 0xffff3fe0U) != 0U) {
                    fputs("FAIL: complete control word\n", stderr);
                    return 1;
                }
                if (devad == 31U && complete != (function ? 0x401fU : 0x001fU)) {
                    fputs("FAIL: literal DP83826I DEVAD 31 words\n", stderr);
                    return 1;
                }
                ++control_cases;
            }
        }
        for (unsigned v = 0; v < sizeof(values) / sizeof(values[0]); ++v) {
            mmdad_reg_t partial;
            partial.val = poison[p];
            partial.adrdata = values[v];
            if (partial.val != ((poison[p] & 0xffff0000U) | values[v])) {
                fputs("FAIL: SDK C data layout differs from audited model\n", stderr);
                return 1;
            }
            uint32_t complete = poison[p];
            complete = values[v];
            if (complete != values[v] || (complete & 0xffff0000U) != 0U) {
                fputs("FAIL: complete 16-bit data word\n", stderr);
                return 1;
            }
            ++data_cases;
        }
    }
    if (hazards != 192U || control_cases != 256U || data_cases != 16U) {
        fputs("FAIL: poison coverage\n", stderr);
        return 1;
    }
    printf("PASS: %u control and %u data vectors; %u poisoned partial-control "
           "hazards reproduced; complete words zero reserved/upper bits\n",
           control_cases, data_cases, hazards);
    puts("Research encoding evidence only; no production driver or SDK link.");
    return 0;
}
