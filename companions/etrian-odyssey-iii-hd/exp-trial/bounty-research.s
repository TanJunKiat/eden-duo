// SPDX-License-Identifier: GPL-3.0-or-later
// Lab telemetry only; no selection, marking, or gold reward. Bit 13 enables
// reporting native start/death/victory evidence. +80 id, +84 raw floor,
// +88 matching death, +92 epoch, +96 last verified victory evidence (u64).
.text
bounty_battle_init:
    mailbox
    stp xzr, xzr, [x17, #80]
    str xzr, [x17, #96]
    stp x29, x30, [sp, #-48]!
bounty_battle_init_return:
    b #0

// The engine has just resolved the encountered native FOE from its live
// dungeon controller. x0 = FoeCtrl, x19 = the resolved native object.
bounty_encounter:
    mailbox
    ldar x16, [x17]
    tbz w16, #13, 1f
    lsr x16, x16, #32
    ldr w15, [x17, #4092]
    cmp w16, w15
    b.ne 1f
    ldr w14, [x0, #16]
    cbnz w14, 1f
    ldr w14, [x0, #88]
    cmp w14, #25
    b.hi 1f
    ldr w13, [x19, #260]
    cbz w13, 1f
    lsr w12, w13, #16
    cbnz w12, 1f
    str w13, [x17, #80]
    str w14, [x17, #84]
    str w15, [x17, #92]
1:
    ldrh w24, [x19, #18]
bounty_encounter_return:
    b #0

// Entry only from the actual enemy-death coroutine (not native summon
// cleanup); preserve native x0/x1 and LR when tail-calling its original target.
bounty_death:
    mailbox
    ldr w16, [x17, #80]
    cbz w16, 1f
    uxth w15, w0
    cmp w15, w16
    b.ne 1f
    str w15, [x17, #88]
1:
bounty_death_call:
    b #0
