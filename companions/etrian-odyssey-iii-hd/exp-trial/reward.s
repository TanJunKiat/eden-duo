// SPDX-License-Identifier: GPL-3.0-or-later
// EO III HD v0.3.0: w0 is the already clamped native battle reward.
// w8 is overwritten below, and x15/x16/x17 are scratch after the native clamp call.
// Mailbox: u64 command, u32 saturated wins at +8, u32 lifecycle epoch at +4092.
// No flags or stack changes. Branch and mailbox address are relocated by Eden.
.text
    adrp x17, 0
    add x17, x17, #0
    mov x15, x17
.ifdef BISTRO_BOUNTY
    ldr x14, [x17, #160]
    str xzr, [x17, #160]
    cbz x14, 7f
    ldr w13, [x17, #4092]
    eor x13, x13, x14, lsr #32
    cbnz w13, 7f
    ldr w12, [x17, #168]
    cbz w12, 7f
    add x11, x17, #144
    ldar x13, [x11]
    eor x13, x13, x14
    cbnz x13, 7f
    ubfx x13, x14, #16, #4
    add w12, w12, w13
    sub w13, w12, #5
    tbnz w13, #31, 6f
    mov w12, #5
6:
    bfi x14, x12, #16, #4
    stlr x14, [x11]
7:
.endif
.ifdef REGULAR_BOUNTY_RESEARCH
    ldr w14, [x17, #132]
    cbz w14, 9f
    str wzr, [x17, #132]
    ldar x14, [x17]
    tbz w14, #14, 9f
    lsr x14, x14, #32
    ldr w13, [x17, #4092]
    eor w14, w14, w13
    cbnz w14, 9f
    ldr w14, [x17, #116]
    cbz w14, 9f
    ldr w12, [x17, #128]
    eor w12, w12, w13
    cbnz w12, 9f
    str wzr, [x17, #116]
    str w14, [x17, #120]
9:
.endif
.ifdef TRAINING_RESEARCH
    // Supersede any prior victory even if this one has no eligible training.
    str xzr, [x15, #16]
    str xzr, [x15, #40]
    str xzr, [x15, #56]
    str w0, [x15, #12]
.endif
.ifdef BOUNTY_RESEARCH
    // The native victory-only path is the final gate: death alone cannot
    // report a bounty. Publish one packed snapshot for read-only lab telemetry.
    ldr w14, [x15, #80]
    cbz w14, 8f
    ldr w13, [x15, #88]
    eor w13, w13, w14
    cbnz w13, 8f
    ldar x13, [x15]
    tbz w13, #13, 8f
    lsr x13, x13, #32
    ldr w12, [x15, #4092]
    eor w13, w13, w12
    cbnz w13, 8f
    ldr w13, [x15, #92]
    eor w13, w13, w12
    cbnz w13, 8f
    ldr w13, [x15, #84]
    orr x14, x14, x13, lsl #32
    add x13, x15, #96
    stlr x14, [x13]
8:
.endif
    ldar x16, [x17]
    lsr x8, x16, #32
    add x17, x17, #4092
    ldar w17, [x17]
    eor w8, w8, w17
    cbnz w8, 5f
    and w17, w16, #0xff
    cbz w17, 1f
    sub w8, w17, #2
    cbz w8, 2f
    sub w8, w17, #5
    cbz w8, 2f
    sub w8, w17, #10
    cbnz w8, 5f
    b 2f
1:
    mov w17, #1
2:
    mul w0, w0, w17
    tbz w16, #8, 4f
    add x15, x15, #8
    ldar w16, [x15]
    sub w8, w16, #4
    cbz w8, 3f
    tbz w8, #31, 4f
    // Reject malformed counts rather than multiplying or changing the streak.
    tbnz w16, #31, 4f
    add w8, w16, #1
    stlr w8, [x15]
3:
    add w8, w16, #4
    mul w0, w0, w8
    lsr w0, w0, #2
4:
    movz w16, #0x423f
    movk w16, #0xf, lsl #16
    sub w8, w16, w0
    tbz w8, #31, 5f
    mov w0, w16
5:
    ldrb w8, [x23, #207]
reward_return:
    b #0
