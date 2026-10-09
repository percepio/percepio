
/*
  dfmCoreDump
  The core part of the DFM_TRAP macro. Fabricates an exception stack frame, 
  just like the processor on a real exception. Passes this to crashcatcher.
  Can be called from main(), RTOS tasks or exceptions.
  
  Floating point values do not display correctly in the core dump.
  
  When fabricating the exception stack frame, the exceptionLR value is
  important. On real exceptions, the processor provides an EXC_RETURN
  code in LR, with status bits that we need to replicate correctly.
  
  EXC_RETURN always begins with 0xFF in the highest bytes, and the two middle
  bytes are not used. The status bits are in the least significant byte.
  
  Currently using 0xFFFFFFB8 as base value, with bit 2 = SPSEL (0 if MSP, 1 if PSP)
  This means no floating point registers are expected.
  
  Lowest byte, bits 0-7, contain various status bits. What should they be?
  
   Bit 0:  ES: Not used by CrashCatcher.
               Arm doc: "Indicates the security domain that is handling the exception. Zero for non-secure, one for secure."
               Not supporting secure mode, so set to 0.

   Bit 1:  Reserved, not used by CC, set to 0.

   bit 2:  SPSEL: Used by CC to decide MSP (=0) or PSP (=1) - seems to give the context of the fault exception.
                  To control CrashCatcher behavior when simulating an exception, we need to assign this bit by reading the SPSEL bit in the CONTROL register. 
                  While this reads the CURRENT mode, this is OK for the DFM_TRAP/dfmCoreDump function, as the handling runs in the same context as the core dump location.
                  Won't work for fault exceptions, but they don't call this function, but the original CrashCatcher entry function and has a proper EXC_RETURN value set by the processor.
                  
                  Arm doc: "Saves the value of CONTROL.SPSEL in the domain that is handling the exception. 
                            The handler is run with CONTROL.SPSEL cleared, to select the Main stack. 
                            The saved value is restored during exception return." 
                  

   Bit 3: Mode:  Not used by CC. 
                 Arm doc: "Indicates which mode was pre-empted. Zero for Handler mode or one for Thread mode."
                 This bit value doesn't matter here, since not used by CrashCatcher.

   Bit 4: Ftype  Used by CC. 
                 Arm doc: "Stack frame type. One for the standard stack frame, or zero for the extended floating-point stack frame format."
                 Should be 1 for CrashCatcher to save all FP regs (then assumes no automated HW stacking).
                 
                 
   Bit 5: DCRS  Not used by CC. 
                Arm doc: "Default callee register stacking used. One when the default rules are used for callee register stacking, 
                          zero when callee register stacking is skipped because the callee registers are already stacked. 
                          This bit is always one if the security extension is not configured."
                 Not used. Set to 1.

   Bit 6: S      Not used by CC.
                 Arm doc: "Indicates whether the exception stack frame is on a secure or a non-secure stack. One means secure. 
                           Always zero if the security extension is not configured. 
                 Set to 0.

   Bit 7-23:     Not used.

   Regarding bit 9 in PSR (PSR_STACK_ALIGN): "on exception entry the core writes a copy of xPSR into the stack frame and
   reuses bit 9 of the stacked xPSR to record whether it inserted a 4-byte pad to force 8-byte alignment."
   Conclusion: This PSR bit always reads as 0 by CrashCatcher when called from dfmCoreDump, so this is fine.
*/


#include "CrashCatcherPriv.h"

    .syntax unified
    .thumb
    .text
    .align 2

    .extern CrashCatcher_Entry
    .extern g_saved_sp
    .extern g_crashCatcherStack
    .extern dfmTrapCallerSavedContext

    .global dfmCoreDump
    .type   dfmCoreDump, %function
    .thumb_func

dfmCoreDump:
    /* Build 8-word exception frame on the original stack to hold
       r0-r3, r12, lr, pc and xpsr. Start by reserving the stack space. */
    sub     sp, sp, #32

    /* r0-r3 and r12 were captured before DFM_TRAP evaluated its metadata.
       Load them from the shared snapshot rather than using the registers
       clobbered by the intervening C code. */
    ldr     r0, =dfmTrapCallerSavedContext
    ldr     r1, [r0, #0]
    str     r1, [sp, #0]
    ldr     r1, [r0, #4]
    str     r1,  [sp, #4]
    ldr     r1, [r0, #8]
    str     r1, [sp, #8]
    ldr     r1, [r0, #12]
    str     r1, [sp, #12]
    ldr     r1, [r0, #16]
    str     r1, [sp, #16]

    /* LR and PC describe the dfmCoreDump call site. */
    str     lr,  [sp, #20]

    /* Store PC = call site, with Thumb bit cleared */
    mov     r1, lr
    sub     r1, r1, #4      /* PC = LR + 4 (size of BL instruction) */
    bic     r1, r1, #1      /* Clear thumb bit (bit 0). PC must be even. */
    str     r1, [sp, #24]

    /* Store xPSR with Thumb bit set */
    mrs     r1, xpsr
    orr     r1, r1, #0x01000000 /* (1 << 24) */
    str     r1, [sp, #28]

    /* Store SP on the heap so it can be restored after CrashCatcher call */
    ldr     r1, =g_saved_sp
    str     sp, [r1]

    /* Switch to CrashCatcher stack, then push the expected registers. */
    mrs     r3, xpsr
    orr     r3, r3, #0x01000000 /* (1 << 24) */
    mrs     r2, psp
    mrs     r1, msp
    ldr.w   r0, =g_crashCatcherStack + (4 * CRASH_CATCHER_STACK_WORD_COUNT)
    mov     sp, r0

    /* Stack the registers CrashCatcher expects (like its fault handler).
       Note: lr is used as a placeholder for exceptionLR, updated below. */
    push.w  {r1-r11, lr}

    /* Update exceptionLR with the EXC_RETURN code (controls stack source). */
    mrs     r1, control
    ubfx    r1, r1, #1, #1      /* SPSEL bit, tells if MSP or PSP stack */

    /* Byte 1 should be B to make CrashCatcher save all FP point regs. */
    movw    r2, #0xFFB8         /* Other bits fixed (lower part) */
    movt    r2, #0xFFFF         /* Other bits fixed (upper part) */
    orr     r2, r2, r1, lsl #2
    str     r2, [sp, #44]       /* Updated placeholder (stacked lr) */

    /* Call CrashCatcher with pointer to the data (sp) */
    mov     r0, sp
    bl      CrashCatcher_Entry

    /* Balance the stack pointer (CrashCatcher part, 12 regs) */
    add     sp, sp, #48

    /* Restore stack pointer and return */
    ldr     r1, =g_saved_sp
    ldr     r1, [r1]            /* r1 = original SP */
    ldr     r0, [r1, #20]       /* r0 = original LR */
    mov     sp, r1              /* Restore stack pointer */
    add     sp, sp, #32         /* Discard fabricated exception frame */

    bx      r0                  /* Return using original LR */

