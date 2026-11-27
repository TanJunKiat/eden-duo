// SPDX-License-Identifier: GPL-3.0-or-later
// ShopSellMenu callsites only: leave native confirmation, inventory and wallet logic intact.
weekly_price:
    ldr w8, [x0]
    stp x8, x30, [sp, #-16]!
weekly_native_price:
    .inst 0x94000000
    ldp x9, x30, [sp], #16
    cmp w0, #1
    b.lt weekly_done
    sub w9, w9, #923
    cmp w9, #1
    b.lo weekly_done
    cmp w9, #551
    b.hi weekly_done
weekly_mailbox:
    adrp x17, 0
    add x17, x17, #0
    ldr x10, [x17, #176]
    ldr w11, [x17, #4092]
    lsr x12, x10, #40
    cmp x11, x12
    b.ne weekly_done
    and w11, w10, #1023
    cmp w9, w11
    b.eq weekly_115
    ubfx x11, x10, #10, #10
    cmp w9, w11
    b.eq weekly_120
    ubfx x11, x10, #20, #10
    cmp w9, w11
    b.eq weekly_130
    ubfx x11, x10, #30, #10
    cmp w9, w11
    b.ne weekly_done
    mov w13, #3
    mov w11, #75
    b weekly_adjust
weekly_115:
    mov w13, #0
    mov w11, #115
    b weekly_adjust
weekly_120:
    mov w13, #1
    mov w11, #120
    b weekly_adjust
weekly_130:
    mov w13, #2
    mov w11, #130
weekly_adjust:
    // Publish the native base price with its item ID in a single word.
    add x14, x17, #184
    orr x12, x0, x9, lsl #32
    str x12, [x14, x13, lsl #3]
    umull x11, w0, w11
    mov w12, #100
    udiv x11, x11, x12
    mov w12, #1
    cmp x11, #0
    csel x11, x11, x12, ne
    mov w12, #0x7fffffff
    cmp x11, x12
    csel w0, w11, w12, lo
weekly_done:
    ret
