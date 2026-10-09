/*
 * Percepio DFM
 * Copyright 2023-2026 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief DFM Crash Catcher integration defines (not used with Zephyr!)
 */

#ifndef DFM_CRASHCATCHER_H
#define DFM_CRASHCATCHER_H

#include <dfmCrashCatcherConfig.h>

#define DFM_TRAP_DUMP_NAME "trap.dmp"
#define DFM_FAULT_DUMP_NAME "fault.dmp"

#ifndef DFM_CFG_ENABLE_COREDUMPS
#define DFM_CFG_ENABLE_COREDUMPS 1
#endif

/**
 * @brief Should call a function that reboots device
 * Example: #define CRASH_STRATEGY_RESET() HAL_NVIC_SystemReset()
 */
#define CRASH_STRATEGY_RESET() NVIC_SystemReset()

/**
 * @brief Endless loop
 * Example: #define CRASH_STRATEGY_LOOP() while(1)
 */
#define CRASH_STRATEGY_LOOP() while(1)

/**
 * @brief Strategy to use after crash has been handled
 * Values: CRASH_STRATEGY_RESET() or CRASH_STRATEGY_LOOP()
 */
#define CRASH_FINALIZE() DFM_DEBUG_PRINT("DFM: Restarting...\n\n\n"); for (volatile int i = 0; i < 1000000; i++); CRASH_STRATEGY_RESET()

#define CRASH_STACK_CAPTURE_SIZE DFM_CFG_STACKDUMP_SIZE

/* Additional memory ranges to include in the crash dump (e.g. heap memory) */
#define CRASH_MEM_REGION1_START	0xFFFFFFFF /* 0xFFFFFFFF = not used */
#define CRASH_MEM_REGION1_SIZE	0

#define CRASH_MEM_REGION2_START	0xFFFFFFFF /* 0xFFFFFFFF = not used */
#define CRASH_MEM_REGION2_SIZE	0

#define CRASH_MEM_REGION3_START	0xFFFFFFFF /* 0xFFFFFFFF = not used */
#define CRASH_MEM_REGION3_SIZE	0

/* CRASH_DUMP_MAX_SIZE - The size of the temporary RAM buffer for crash dumps.
 * Any attempt to write outside this buffer will be caught in CrashCatcher_DumpMemory() and give an error.
 * Enable DFM_CFG_USE_DEBUG_LOGGING to see the actual usage.
 * */
#ifndef DFM_CFG_MAX_COREDUMP_SIZE
#define DFM_CFG_MAX_COREDUMP_SIZE (300 + (CRASH_STACK_CAPTURE_SIZE) + (CRASH_MEM_REGION1_SIZE) + (CRASH_MEM_REGION2_SIZE) + (CRASH_MEM_REGION3_SIZE))
#endif

#define CRASH_DUMP_BUFFER_SIZE DFM_CFG_MAX_COREDUMP_SIZE

typedef struct{
	int alertType;
	char* message;
	char* file;
	int line;
	int restart;
} dfmTrapInfo_t;

extern dfmTrapInfo_t dfmTrapInfo;

#include <dfmUtility.h>

extern __attribute__ ((naked)) void dfmCoreDump(void);
  
// void dfmStackOverflowCheckSuspend(void) and Resume()
// Used in DFM_TRAP() to suspend and resume stack overflow checking in ArmV8-M
// processors using the CMSIS Core register access functions. These are defined 
// also for older Arm processors lacking stack supervision (like Cortex-M4) and
// then has no effect.
// This is necessary since CrashCatcher updates the stack pointer to use a 
// separate stack to minimize the risk of stack overflows in CrashCatcher. 
// Ironically, this stack switch can trigger the stack overflow detection in
// ArmV8-M processors, if enabled and the CrashCatcher stack is at a lower
// address than the current stack limit (PSPLIM or MSPLIM).

void dfmStackOverflowCheckSuspend(void);
void dfmStackOverflowCheckResume(void);
void vDfmCrashCatcherClearTrapInfo(void);
void vDfmCrashCatcherAlertOnly(void);

/* The byte pattern used in DFM_STACK_MARKER. */
#define DFM_STACK_MARKER_MAGIC_STR "coredump_end"
 
/******************************************************************************
 * DFM_TRAP(alertType, message, restart_flag)
 *
 * Allows for manually triggering a core dump from code, e.g. in error handling
 * code. This can be called from all contexts - from tasks, exceptions and in
 * the startup code (main). Note that fault exceptions doesn't require using
 * DFM_TRAP(), as they are captured by the CrashCatcher fault handler.
 *  
 * Arguments:
 *
 *   - alertType: An integer code for the alert type, matching the Detect
 *                 server configuration (normally to dfmCodes.h).
 *   - message: A string with additional details about the fault.
 *
 *   - restart_flag: If 1, the processor is restarted afterwards.
 *                   If 0, the execution resumes.
 * Example:
 *
 *  if (arg1 < 0) {
 *     DFM_TRAP(DFM_TYPE_ASSERT_FAILED, "Argument arg1 < 0", 0); // No restart
 *     return ERR;
 *  }
 *  
 * Note: On ArmV8-M devices like Cortex M33, the hardware stack overflow
 * checking might kick in on DFM_TRAP() since CrashCatcher jumps to
 * its own stack. If the CC stack happens to be at a lower address than PSPLIM
 * (or MSPLIM) it will appear as a stack overflow a fault exception. 
 * So if PSPLIM or MSPLIM are used (non-zero), they must be set to 0 before 
 * DFM_TRAP is called (and restored if no restart).
 *****************************************************************************/
#if ((DFM_CFG_ENABLE_COREDUMPS) >= 1)
#define DFM_TRAP_PROCESS()             \
  do {                                                                        \
    dfmStackOverflowCheckSuspend();                                           \
    dfmCoreDump();                                                            \
    dfmStackOverflowCheckResume();                                            \
  } while (0)
#else
#define DFM_TRAP_PROCESS() vDfmCrashCatcherAlertOnly()
#endif

#define DFM_TRAP(alertType, message, restart_flag)                            \
  do {                                                                        \
    DFM_TRAP_SAVE_ARGS(alertType, message, __FILE__, __LINE__, restart_flag); \
    if (ulDfmIsInitialized() != 0UL) {                                        \
      DFM_TRAP_PROCESS();                                                     \
    } else {                                                                  \
      vDfmCrashCatcherClearTrapInfo();                                        \
    }                                                                         \
  } while (0)

/******************************************************************************
 * DFM_STACK_MARKER()
 * Put this first in the task entry function to crop the stack dumps, reducing
 * their size and avoiding GDB warnings from confused stack unwinding.
 *****************************************************************************/
#define DFM_STACK_MARKER() \
    __attribute__((aligned(4))) \
    volatile char dfm_stack_marker[] = DFM_STACK_MARKER_MAGIC_STR; \
    (void)dfm_stack_marker; // To avoid "unused" warnings



#endif
