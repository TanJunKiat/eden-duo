// SPDX-License-Identifier: GPL-3.0-or-later
// Lab only. Bit 12 enables Favor; +64 RNG, +68 pending, +72 pending epoch.
// Only the completed AM/PM paid-stay callbacks advance the private RNG.
.macro paid_roll
    mailbox
    ldar x16, [x17]
    tbz w16, #12, 1f
    lsr x16, x16, #32
    ldr w15, [x17, #4092]
    cmp w16, w15
    b.ne 1f
    ldr w14, [x17, #64]
    cbz w14, 1f
    eor w14, w14, w14, lsl #13
    eor w14, w14, w14, lsr #17
    eor w14, w14, w14, lsl #5
    str w14, [x17, #64]
    tst w14, #3
    b.ne 1f
    str w15, [x17, #72]
    mov w14, #1
    str w14, [x17, #68]
1:
    str d10, [sp, #-80]!
.endm
.text
favor_paid_am:
    paid_roll
favor_paid_am_return:
    b #0

favor_paid_pm:
    paid_roll
favor_paid_pm_return:
    b #0

// Both callers have already chosen Yes. x20 is the native sale list,
// x0 the shop, w1 its ordinary quoted credit. Retain the original LR and
// tail-call Sale with the adjusted price only for a fully eligible list.
favor_sale:
    mailbox
    ldar x16, [x17]
    tbz w16, #12, 7f
    lsr x16, x16, #32
    ldr w15, [x17, #4092]
    cmp w16, w15
    b.ne 7f
    ldr w14, [x17, #72]
    cmp w14, w15
    b.ne 7f
    ldr w14, [x17, #68]
    cmp w14, #1
    b.ne 7f
    cmp w1, #2
    b.lt 7f
    movz w14, #0xe0ff
    movk w14, #0x05f5, lsl #16
    cmp w1, w14
    b.hi 7f
    cbz x20, 7f
    ldr w15, [x20, #24]
    sub w14, w15, #1
    cmp w14, #59
    b.hi 7f
    ldr x14, [x20, #16]
    cbz x14, 7f
    ldr w13, [x14, #24]
    cmp w15, w13
    b.hi 7f
    stp x29, x30, [sp, #-80]!
    mov x29, sp
    stp x0, x1, [sp, #24]
    stp x22, x23, [sp, #40]
    stp x24, x25, [sp, #56]
    add x22, x14, #32
    mov w23, w15
    mov w24, wzr
2:
    ldrsh w8, [x22, x24, lsl #0]
    sub w9, w8, #924
    cmp w9, #550
    b.hi 5f
    str w8, [sp, #16]
    add x0, sp, #16
    mov x1, xzr
favor_material_call:
    bl #0
    cbz w0, 5f
    add w24, w24, #8
    cmp w24, w23, lsl #3
    b.lo 2b
    ldp x0, x1, [sp, #24]
    lsr w8, w1, #1
    mov w9, #5000
    cmp w8, w9
    csel w8, w8, w9, lo
    add w1, w1, w8
favor_consume_mailbox:
    mailbox
    str wzr, [x17, #68]
    b 6f
5:
    ldp x0, x1, [sp, #24]
6:
    ldp x22, x23, [sp, #40]
    ldp x24, x25, [sp, #56]
    ldp x29, x30, [sp], #80
7:
favor_sale_call:
    b #0
