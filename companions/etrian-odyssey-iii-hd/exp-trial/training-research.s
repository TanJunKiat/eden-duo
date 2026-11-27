// SPDX-License-Identifier: GPL-3.0-or-later
// Lab only. Assemble after reward.s with TRAINING_RESEARCH defined.
// Mailbox: +12 raw total, +16 result packet, +24 prepared share, +28 epoch,
// +32 scoped share, +36 prepared selector. Rotation uses bit 11 and +40
// party, +48 half-share, +56 result packet. All are distinct from Boost/Deep Run.
.macro mailbox
    adrp x17, 0
    add x17, x17, #0
.endm
.text
training_split:
    mailbox
    str xzr, [x17, #16]
    ldar x16, [x17]
    ldr w15, [x17, #4092]
    lsr x14, x16, #32
    cmp w14, w15
    b.ne 3f
    sub w14, w21, #1
    cmp w14, #4
    b.hi 3f
    ldr w14, [x17, #12]
    str wzr, [x17, #12]
    sub w13, w14, #1
    movz w12, #0x423e
    movk w12, #0xf, lsl #16
    cmp w13, w12
    b.hi 3f
    udiv w14, w14, w21
    cbnz w14, 1f
    mov w14, #1
1:
    str w15, [x17, #28]
    tbz w16, #11, 4f
    lsr w13, w14, #1
    str w13, [x17, #48]
    str x8, [x17, #56]
4:
    ubfx w16, w16, #9, #2
    cbz w16, 3f
    cmp w16, #2
    b.hi 3f
    tbz w16, #0, 2f
    lsr w14, w14, #1
2:
    str w14, [x17, #24]
    str w16, [x17, #36]
    str x8, [x17, #16]
3:
    ldr w8, [x8, #104]
training_split_return:
    b #0

// Wrap only the result-opening call. The record is consumed before invoking
// the original function and the scope ends immediately on return.
training_result:
    mailbox
    str wzr, [x17, #32]
    ldr x16, [x17, #16]
    str xzr, [x17, #16]
    cbz x16, 1f
    ldr x15, [x19, #16]
    cbz x15, 1f
    ldr x15, [x15, #200]
    cmp x15, x16
    b.ne 1f
    ldar x16, [x17]
    lsr x15, x16, #32
    ldr w14, [x17, #4092]
    cmp w15, w14
    b.ne 1f
    ldr w13, [x17, #28]
    cmp w13, w14
    b.ne 1f
    ubfx w16, w16, #9, #2
    cbz w16, 1f
    ldr w13, [x17, #36]
    cmp w16, w13
    b.ne 1f
    ldr w14, [x17, #24]
    str w14, [x17, #32]
1:
training_result_call:
    bl #0
training_result_clear:
    mailbox
    str wzr, [x17, #32]
training_result_return:
    b #0

// Native TST has already determined whether Combat Study was learned.
training_untrained:
    b.ne 1f
training_untrained_mailbox:
    mailbox
    ldr w11, [x17, #32]
    cbz w11, 2f
    ldr x8, [x23]
    cbz x8, 2f
    ldrb w12, [x8, #16]
    and w12, w12, #3
    cmp w12, #1
    b.ne 2f
    ldrb w12, [x8, #19]
    cbz w12, 2f
    ldr x12, [x8, #88]
    cbz x12, 2f
    ldr w12, [x12, #16]
    cbz w12, 2f
    ldr w10, [x8, #84]
    mov w9, w11
    add w9, w10, w9
training_untrained_store:
    b #0
1:
training_untrained_return:
    b #0
2:
training_untrained_skip:
    b #0

// w9 is the native award (not accumulated EXP), w10 is the existing total.
training_trained:
    mailbox
    ldr w11, [x17, #32]
    cbz w11, 1f
    cmp w9, w11
    csel w9, w9, w11, hs
1:
    add w9, w10, w9
training_trained_store:
    b #0

// Preserve both native arguments and LR. Only this victory's normal split can
// authorize rotation using this freshly retrieved party object.
rotation_party:
    mailbox
    str x0, [x17, #40]
rotation_party_call:
    b #0

rotation_finish:
    mailbox
    ldr x16, [x17, #56]
    cbz x16, 7f
    str xzr, [x17, #56]
    ldr x15, [x19, #200]
    cmp x15, x16
    b.ne 7f
    ldar x15, [x17]
    tbz w15, #11, 7f
    lsr x15, x15, #32
    ldr w14, [x17, #4092]
    cmp w15, w14
    b.ne 7f
    ldr w15, [x17, #28]
    cmp w15, w14
    b.ne 7f
    ldr w13, [x17, #48]
    cbz w13, 7f
    ldr x15, [x17, #40]
    cbz x15, 7f
    ldr x14, [x15, #40]
    cbz x14, 7f
    ldr w11, [x14, #24]
    sub w15, w11, #1
    cmp w15, #4
    b.hi 7f
    ldr x12, [x16, #112]
    cbz x12, 7f
    ldr w15, [x12, #24]
    cmp w15, #5
    b.ne 7f
    add x14, x14, #32
    add x12, x12, #32
    movz w3, #0x423f
    movk w3, #0xf, lsl #16
    mov w10, wzr
    mov w9, wzr
    mov x7, xzr
    // Validate all entries before modifying any. Pack living levels only;
    // invalid bounds, status or an unexpected native award are no-ops.
1:
    ldr x5, [x14, x10, lsl #3]
    cbz x5, 7f
    ldrb w4, [x5, #16]
    and w4, w4, #3
    cmp w4, #3
    b.ne 7f
    ldr w4, [x12, x10, lsl #2]
    ldrb w8, [x5, #96]
    tbz w8, #0, 2f
    cbnz w4, 7f
    b 3f
2:
    cbz w4, 7f
    cmp w4, w3
    b.hi 7f
    ldrb w8, [x5, #19]
    sub w4, w8, #1
    cmp w4, #98
    b.hi 7f
    cmp w8, w9
    csel w9, w8, w9, hi
    lsl w6, w10, #3
    lsl x8, x8, x6
    orr x7, x7, x8
3:
    add w10, w10, #1
    cmp w10, w11
    b.lo 1b
    mov w10, wzr
4:
    and w8, w7, #0xff
    cbz w8, 5f
    cmp w8, w9
    b.hs 5f
    ldr w4, [x12, x10, lsl #2]
    add w4, w4, w13
    cmp w4, w3
    csel w4, w4, w3, lo
    str w4, [x12, x10, lsl #2]
5:
    lsr x7, x7, #8
    add w10, w10, #1
    cmp w10, w11
    b.lo 4b
7:
    ldp x20, x19, [sp, #96]
rotation_finish_return:
    b #0
