// SPDX-License-Identifier: GPL-3.0-or-later
// Research variant: bit 15 and mailbox +136 arm a disposable one-shot.
// Release variant: +144 contains species, kills, claim flags and epoch;
// +152 is the native party identity captured on acceptance. No lab trigger.
// This hook creates a fresh native town result. The game applies all EXP
// itself when the result is confirmed; no existing quest packet is changed.
.text
bistro_claim:
    mailbox
.ifdef BISTRO_BOUNTY
    add x14, x17, #144
    ldar x16, [x14]
    tbz w16, #20, .Lclaim_original
    tbnz w16, #21, .Lclaim_original
    uxth w15, w16
    cbz w15, .Lclaim_original
    ubfx w15, w16, #16, #4
    cmp w15, #5
    b.ne .Lclaim_original
    ldr w15, [x17, #4092]
    lsr x13, x16, #32
    cmp w15, w13
    b.ne .Lclaim_original
    // Leave the request set while the game constructs the result, so a host
    // sample cannot expose a second claim button during this callback.
.else
    ldar x16, [x17]
    tbz w16, #15, .Lclaim_original
    lsr x16, x16, #32
    ldr w15, [x17, #4092]
    cmp w16, w15
    b.ne .Lclaim_original
    ldr w15, [x17, #136]
    cmp w15, #1
    b.ne .Lclaim_original
    str wzr, [x17, #136]
    str w15, [x17, #140]
.endif
    stp x29, x30, [sp, #-112]!
    mov x29, sp
    stp x19, x20, [sp, #16]
    stp x21, x22, [sp, #32]
    stp x23, x24, [sp, #48]
    stp x25, x26, [sp, #64]
    stp x27, x28, [sp, #80]
    stp x0, x1, [sp, #96]
    mov x19, x0
    mov x0, xzr
bistro_party_call:
    bl #0
    cbz x0, .Lclaim_fail
    ldr x20, [x0]
    cbz x20, .Lclaim_fail
.ifdef BISTRO_BOUNTY
bistro_party_mailbox:
    mailbox
    ldr x8, [x17, #152]
    cmp x20, x8
    b.ne .Lclaim_fail
.endif
    ldr x20, [x20, #40]
    cbz x20, .Lclaim_fail
    ldr w21, [x20, #24]
    sub w8, w21, #1
    cmp w8, #4
    b.hi .Lclaim_fail
    mov x0, xzr
    mov w1, wzr
bistro_packet_call:
    bl #0                         // native DividedExp(0) allocates the packet
    cbz x0, .Lclaim_fail
    mov x22, x0
    ldr x23, [x22, #112]
    cbz x23, .Lclaim_fail
    ldr w8, [x23, #24]
    cmp w8, #5
    b.ne .Lclaim_fail
    add x23, x23, #32
    mov w24, wzr                   // slot
    mov w25, wzr                   // EXP caption total
.Lclaim_member:
    add x8, x20, x24, lsl #3
    ldr x8, [x8, #32]
    cbz x8, .Lclaim_fail
    ldrb w9, [x8, #16]
    and w9, w9, #3
    cmp w9, #3
    b.ne .Lclaim_fail
    ldrb w26, [x8, #19]
    ldrb w9, [x8, #98]
    cbz w26, .Lclaim_fail
    cmp w9, #99
    b.hi .Lclaim_fail
    cmp w26, w9
    b.hi .Lclaim_fail
    b.eq .Lclaim_next
    ldr w27, [x8, #20]
    mov w0, w27
    mov w1, w26
    mov x2, xzr
bistro_current_exp_call:
    bl #0
    tbnz w0, #31, .Lclaim_fail
    mov w28, w0
    mov w0, w27
    add w1, w26, #1
    mov x2, xzr
bistro_next_exp_call:
    bl #0
    cmp w0, w28
    b.le .Lclaim_fail
    sub w0, w0, w28
    mov w8, #10
    udiv w0, w0, w8
    str w0, [x23, x24, lsl #2]
    add w25, w25, w0
.Lclaim_next:
    add w24, w24, #1
    cmp w24, w21
    b.lo .Lclaim_member
    str w25, [x22, #104]
    cbz w25, .Lclaim_empty

    // Resolve both generic method metadata slots using the same initialization
    // routine used in the game's own scripted town-result launcher.
bistro_town_metadata:
    adrp x24, 0
    ldr x24, [x24, #0]
    cbz x24, .Lclaim_fail
    mov x0, x24
bistro_resolve_town_call:
    bl #0
    ldr x0, [x24]
bistro_town_instance_call:
    bl #0
    cbz x0, .Lclaim_fail
    ldr x8, [x0, #24]
    cbz x8, .Lclaim_fail
    ldp x25, x26, [x8, #56]
    cbz x25, .Lclaim_fail
bistro_result_metadata:
    adrp x24, 0
    ldr x24, [x24, #0]
    cbz x24, .Lclaim_fail
    mov x0, x24
bistro_resolve_result_call:
    bl #0

    // Build the same Report action the native Bistro already uses. On
    // callback after result, the consumed request falls through to vanilla.
bistro_action_type:
    adrp x8, 0
    ldr x8, [x8, #0]
    cbz x8, .Lclaim_fail
    ldr x0, [x8]
    cbz x0, .Lclaim_fail
bistro_alloc_action_call:
    bl #0
    cbz x0, .Lclaim_fail
    mov x27, x0
bistro_report_metadata:
    adrp x8, 0
    ldr x8, [x8, #0]
    cbz x8, .Lclaim_fail
    ldr x2, [x8]
    cbz x2, .Lclaim_fail
    mov x1, x19
    mov x3, xzr
bistro_action_ctor_call:
    bl #0

    mov x0, x25
    mov x1, x26
    ldr x2, [x24]
bistro_instantiate_call:
    bl #0
    cbz x0, .Lclaim_fail
    mov x21, x0
    strb wzr, [x21, #352]          // town result
    str x20, [x21, #312]
    add x0, x21, #312
    mov x1, x20
bistro_roster_barrier_call:
    bl #0
    str x22, [x21, #344]
    add x0, x21, #344
    mov x1, x22
bistro_packet_barrier_call:
    bl #0
    str x27, [x21, #304]
    add x0, x21, #304
    mov x1, x27
bistro_callback_barrier_call:
    bl #0
bistro_result_mailbox:
    mailbox
.ifdef BISTRO_BOUNTY
    add x14, x17, #144
    ldar x16, [x14]
    bic x16, x16, #0x100000
    orr x16, x16, #0x200000
    stlr x16, [x14]
.else
    mov w16, #2
    str w16, [x17, #140]
.endif
    mov w16, #1
    b .Lclaim_done
.Lclaim_empty:
.ifdef BISTRO_BOUNTY
bistro_empty_mailbox:
    mailbox
    add x14, x17, #144
    ldar x16, [x14]
    bic x16, x16, #0x100000
    orr x16, x16, #0x200000
    stlr x16, [x14]
    b .Lclaim_forward
.endif
.Lclaim_fail:
.ifdef BISTRO_BOUNTY
bistro_failed_mailbox:
    mailbox
    add x14, x17, #144
    ldar x16, [x14]
    bic x16, x16, #0x100000
    stlr x16, [x14]
.endif
.Lclaim_forward:
    mov w16, wzr
    ldp x0, x1, [sp, #96]
.Lclaim_done:
    ldp x27, x28, [sp, #80]
    ldp x25, x26, [sp, #64]
    ldp x23, x24, [sp, #48]
    ldp x21, x22, [sp, #32]
    ldp x19, x20, [sp, #16]
    ldp x29, x30, [sp], #112
    cbz w16, .Lclaim_original
    ret
.Lclaim_original:
    stp x29, x30, [sp, #-48]!
bistro_original_return:
    b #0
