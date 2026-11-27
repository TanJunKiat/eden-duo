// SPDX-License-Identifier: GPL-3.0-or-later
// Lab-only probe. Bit 14 arms telemetry for the current mailbox epoch.
// +112 selected species (zero observes every species), +116 pending count,
// +120 last victory count, +124 last observed species, +128 pending epoch,
// +132 ordinary encounter authorization, consumed by victory.
// Never changes rewards.
.text
regular_battle_init:
    mailbox
.ifdef BISTRO_BOUNTY
    str xzr, [x17, #160]
    str wzr, [x17, #168]
.else
    str wzr, [x17, #116]
    str wzr, [x17, #120]
    str wzr, [x17, #124]
    str wzr, [x17, #132]
.endif
    stp x29, x30, [sp, #-48]!
regular_battle_init_return:
    b #0

// Only the random labyrinth encounter branch of BattleInit reaches this
// patched call. The FOE branch jumps past it; other modes use other setup.
regular_normal_encounter:
    mailbox
.ifdef BISTRO_BOUNTY
    add x16, x17, #144
    ldar x16, [x16]
    str x16, [x17, #160]
.else
    mov w16, #1
    str w16, [x17, #132]
.endif
regular_normal_encounter_call:
    b #0

// BattleUnitDeadProc is about to call BattleOutAddDeadEnemy. Preserve all
// arguments and the original LR. w0 carries the complete species halfword.
regular_death:
    mailbox
.ifdef BISTRO_BOUNTY
    ldr x16, [x17, #160]
.ifdef ARMOROAD_ROOMS
    tbnz w16, #22, 1f            // paused contract
.endif
    uxth w15, w0
    uxth w14, w16
    cbz w14, 1f
    cmp w15, w14
    b.ne 1f
    ldr w15, [x17, #4092]
    lsr x14, x16, #32
    cmp w15, w14
    b.ne 1f
    ubfx w14, w16, #16, #4
    cmp w14, #5
    b.hs 1f
    ldr w15, [x17, #168]
    cmp w15, #5
    b.hs 1f
    add w15, w15, #1
    str w15, [x17, #168]
.else
    ldr w15, [x17, #132]
    cbz w15, 1f
    ldar x16, [x17]
    tbz w16, #14, 1f
    lsr x16, x16, #32
    ldr w15, [x17, #4092]
    cmp w16, w15
    b.ne 1f
    uxth w14, w0
    cbz w14, 1f
    str w14, [x17, #124]
    ldr w15, [x17, #112]
    cbz w15, 2f
    cmp w14, w15
    b.ne 1f
2:
    ldr w15, [x17, #128]
    cmp w15, w16
    b.eq 3f
    str wzr, [x17, #116]
    str w16, [x17, #128]
3:
    ldr w15, [x17, #116]
    cmp w15, #5
    b.hs 1f
    add w15, w15, #1
    str w15, [x17, #116]
.endif
1:
regular_death_call:
    b #0
