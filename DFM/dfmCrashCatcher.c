/*
 * Percepio DFM
 * Copyright 2023-2026 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * DFM Crash Catcher integration
 */

#include <stdint.h>
#include <string.h>
#include <CrashCatcher.h>
#include <dfm.h>
#include <dfmCrashCatcher.h>
#include <CrashCatcherPriv.h>
#include <dfmKernelPort.h>

#if ((DFM_CFG_ENABLED) >= 1)

#if ((DFM_CFG_CRASH_ADD_TRACE) >= 1)
#include <trcRecorder.h>
static void prvAddTracePayload(void);
#endif

/* See https://developer.arm.com/documentation/dui0552/a/cortex-m3-peripherals/system-control-block/configurable-fault-status-register*/
#define ARM_CORTEX_M_CFSR_REGISTER *(uint32_t*)0xE000ED28

static DfmAlertHandle_t xAlertHandle = 0;
static uint32_t ulCoredumpOverflowed = 0U;

// CrashCatcher changes stack, so we need to restore sp to allow returning from CrashCatcher.
volatile uint32_t g_saved_sp = 0; 

dfmTrapInfo_t dfmTrapInfo = {-1, (void*)0, (void*)0, -1, 0};
volatile DfmTrapCallerSavedContext_t dfmTrapCallerSavedContext = {0};

#if !defined(__ICCARM__)
/*
 * Capture r0-r3 and r12 before DFM_TRAP evaluates or stores its metadata.
 * This naked function contains only one basic assembly statement. Its stack
 * and scratch-register changes are therefore fully balanced before control
 * returns to compiler-generated C code. The IAR implementation is located in
 * the existing dfmCoreDump-IAR.S file.
 */
void __attribute__((naked, noinline)) dfmTrapCaptureCallerSavedContext(void)
{
	__asm volatile (
		"push {r0-r3}\n"
		"ldr r0, =dfmTrapCallerSavedContext\n"
		"ldr r1, [sp, #0]\n"
		"str r1, [r0, #0]\n"
		"ldr r1, [sp, #4]\n"
		"str r1, [r0, #4]\n"
		"ldr r1, [sp, #8]\n"
		"str r1, [r0, #8]\n"
		"ldr r1, [sp, #12]\n"
		"str r1, [r0, #12]\n"
		"mov r1, r12\n"
		"str r1, [r0, #16]\n"
		"pop {r0-r3}\n"
		"bx lr\n"
	);
}
#endif

#if ((DFM_CFG_CRASH_ADD_TRACE) >= 1)
static TraceStringHandle_t TzUserEventChannel = 0;
static uint32_t uiTraceWasEnabledAtDumpStart = 0U;
#endif

uintptr_t __stack_chk_guard = 0xDEADBEEF;

static uint8_t* ucBufferPos;
static uint8_t ucDataBuffer[CRASH_DUMP_BUFFER_SIZE] __attribute__ ((aligned (8)));

static void dumpHalfWords(const uint16_t* pMemory, size_t elementCount);
static void dumpWords(const uint32_t* pMemory, size_t elementCount);

void vDfmCrashCatcherClearTrapInfo(void)
{
	dfmTrapInfo.alertType = -1;
	dfmTrapInfo.message = (void*)0;
	dfmTrapInfo.restart = 0;
	dfmTrapInfo.file = (void*)0;
	dfmTrapInfo.line = 0;
}

void vDfmCrashCatcherAlertOnly(void)
{
	DfmAlertHandle_t xLocalAlertHandle = (void*)0;
	const char *szFileName = szDfmGetFileNameFromPath(dfmTrapInfo.file);
	const int restart = dfmTrapInfo.restart;
#if ((DFM_CFG_CRASH_ADD_TRACE) >= 1)
	uint32_t uiRecorderNeedsResume = 0U;
#endif

	snprintf(cDfmPrintBuffer, sizeof(cDfmPrintBuffer), "%s at %s:%u",
		dfmTrapInfo.message, szFileName, dfmTrapInfo.line);

	DFM_CFG_PRINT(LNBR "DFM Alert: ");
	DFM_CFG_PRINT(cDfmPrintBuffer);
	DFM_CFG_PRINT(LNBR);

	if (xDfmAlertBegin((uint32_t)dfmTrapInfo.alertType, cDfmPrintBuffer,
		&xLocalAlertHandle) == DFM_SUCCESS)
	{
		(void)xDfmAddFileAndLineSymptoms(xLocalAlertHandle, szFileName,
			dfmTrapInfo.line);
#if ((DFM_CFG_CRASH_ADD_TRACE) >= 1)
		if (xTraceIsRecorderEnabled())
		{
			if (TzUserEventChannel == 0)
			{
				(void)xTraceStringRegister("ALERT", &TzUserEventChannel);
			}
			(void)xTracePrint(TzUserEventChannel, cDfmPrintBuffer);
			(void)vDfmAddTracePayload(xLocalAlertHandle);
			pxTraceRecorderData->uiRecorderEnabled = 0U;
			uiRecorderNeedsResume = 1U;
		}
#endif
#ifdef DFM_CLOUD_PORT_ALWAYS_ATTEMPT_TRANSFER
		(void)xDfmAlertEnd(xLocalAlertHandle);
#else
		(void)xDfmAlertEndOffline(xLocalAlertHandle);
#endif
	}

	vDfmCrashCatcherClearTrapInfo();
	if (restart == 1)
	{
		CRASH_FINALIZE();
	}
#if ((DFM_CFG_CRASH_ADD_TRACE) >= 1)
	if (uiRecorderNeedsResume != 0U)
	{
		pxTraceRecorderData->uiRecorderEnabled = 1U;
	}
#endif
}

uint32_t stackPointer = 0;

const CrashCatcherMemoryRegion* CrashCatcher_GetMemoryRegions(void)
{
	static CrashCatcherMemoryRegion regions[] = {
		{0xFFFFFFFF, 0xFFFFFFFF, CRASH_CATCHER_BYTE},
		{CRASH_MEM_REGION1_START, CRASH_MEM_REGION1_START + CRASH_MEM_REGION1_SIZE, CRASH_CATCHER_BYTE},
		{CRASH_MEM_REGION2_START, CRASH_MEM_REGION2_START + CRASH_MEM_REGION2_SIZE, CRASH_CATCHER_BYTE},
		{CRASH_MEM_REGION3_START, CRASH_MEM_REGION3_START + CRASH_MEM_REGION3_SIZE, CRASH_CATCHER_BYTE},
		{0xFFFFFFFF, 0xFFFFFFFF, CRASH_CATCHER_BYTE}
	};
	/* Make sure the regions list is valid (start address < end address). 
	 * Stop at the first unused entry. If endAddress wrapped, or SIZE was zero, endAddress is not greater
	 * than startAddress and the entry must not be passed to CrashCatcher. */
	for (int i = 1; i <= 3; i++)
	{
		if (regions[i].startAddress == 0xFFFFFFFFU)
		{
			break;
		}

		if (regions[i].endAddress <= regions[i].startAddress)
		{
			regions[i].startAddress = 0xFFFFFFFFU;
			break;
		}
	}

	/* Region 0 is reserved, always relative to the current stack pointer */
	regions[0].startAddress = (uint32_t)stackPointer;

	/* The configured readable range is [BEGIN, NEXT), where NEXT is the first
	 * address that must not be read. If SP is outside this range, skip only the
	 * stack region. Returning regions[1] preserves any configured extra regions. */
	if (regions[0].startAddress < DFM_CFG_ADDR_CHECK_BEGIN)
	{
		return &regions[1];
	}
        
	if (regions[0].startAddress >= DFM_CFG_ADDR_CHECK_NEXT)
	{
		return &regions[1];
	}

	/* Calculate the number of readable bytes without adding to startAddress. */
	uint32_t availableBytes =
		DFM_CFG_ADDR_CHECK_NEXT - regions[0].startAddress;

	if (CRASH_STACK_CAPTURE_SIZE >= availableBytes)
		{
		/* endAddress is exclusive, so it may equal the first invalid address. */
		regions[0].endAddress = DFM_CFG_ADDR_CHECK_NEXT;
		}
	else
	{
		/* The comparison above proves that this addition cannot wrap or pass
		 * DFM_CFG_ADDR_CHECK_NEXT. */
		regions[0].endAddress =
			regions[0].startAddress + CRASH_STACK_CAPTURE_SIZE;
	}

        /* Check 2 - Limit the dump to DFM_STACK_MARKER (truncate if found) */

        /* strlen excludes the terminating NUL. The marker text identifies the
         * marker; the NUL is included in the dump below only when it fits. */
        const size_t pattern_len = strlen(DFM_STACK_MARKER_MAGIC_STR);
        /* Start at SP itself; rounding down could read bytes below the validated
         * dump range. The stack and marker are word-aligned, hence the step of 4. */
        uintptr_t addr = (uintptr_t)regions[0].startAddress;
        uintptr_t endaddr = (uintptr_t)regions[0].endAddress;
        while (addr < endaddr)
        {
            /* Subtraction is safe because addr is not above endaddr. */
            uintptr_t bytes_remaining = endaddr - addr;

            /* memcmp() may only run when the complete marker fits below the
             * exclusive end address. */
            if (bytes_remaining < pattern_len)
            {
                break;
            }

            /* Scan the stack for DFM_STACK_MARKER, from low adress (current SP)
             * to high address (start of stack). If this byte pattern is found
             * within the dump window, truncate the stack dump right after that.
             * This assumes that DFM_STACK_MARKER() is added first in the
             * task entry function (or in main).*/
            
            void* p_addr = (void*)addr; // May give a warning, but hard to avoid.
          
            if (memcmp(p_addr, DFM_STACK_MARKER_MAGIC_STR, pattern_len) == 0)
            {
                uint32_t markerEnd =
                    (uint32_t)addr + (uint32_t)pattern_len;

                /* Include the terminating NUL without extending the region
                 * past its already validated exclusive end address. */
                if (markerEnd < regions[0].endAddress)
                {
                    markerEnd++;
                }
                regions[0].endAddress = markerEnd;
                break;
            }
            addr += 4;
        }
        
	return regions;
}
void CrashCatcher_DumpStart(const CrashCatcherInfo* pInfo)
{
	int alerttype;
	const char* szFileName = (void*)0;
	char* szCurrentTaskName = (void*)0;

	stackPointer = pInfo->sp;

	ucBufferPos = &ucDataBuffer[0];
	ulCoredumpOverflowed = 0U;
	xAlertHandle = (void*)0;

	CC_DBG_LOG("CrashCatcher_DumpStart" LNBR);

	if (dfmTrapInfo.alertType >= 0)
	{
		/* Called on DFM_TRAP calls.
		 * DFM_TRAP stores its metadata in dfmTrapInfo before dfmCoreDump:
		 * dfmTrapInfo.message = "Assert failed" or similar.
		 * dfmTrapInfo.file = __FILE__ (full path, extract the filename from this!)
		 * dfmTrapInfo.line = __LINE__ (integer)
		 * */
		szFileName = szDfmGetFileNameFromPath(dfmTrapInfo.file);
		snprintf(cDfmPrintBuffer, sizeof(cDfmPrintBuffer), "%s at %s:%u", dfmTrapInfo.message, szFileName, dfmTrapInfo.line);

		alerttype = dfmTrapInfo.alertType;
	}
	else
	{
		/* On processor fault handlers (not DFM_TRAP) */
		snprintf(cDfmPrintBuffer, sizeof(cDfmPrintBuffer), "Fault exception, CFSR: 0x%08X", (unsigned int)ARM_CORTEX_M_CFSR_REGISTER);

		alerttype = DFM_TYPE_HARDFAULT;
	}

#if ((DFM_CFG_CRASH_ADD_TRACE) >= 1)
	/* Keep tracing enabled until DumpEnd so all alert-related TraceRecorder
	 * calls can log before the event buffer is saved. */
	uiTraceWasEnabledAtDumpStart = 0U;
	if (xTraceIsRecorderEnabled())
    {
		uiTraceWasEnabledAtDumpStart = 1U;
		if (TzUserEventChannel == 0)
		{
			xTraceStringRegister("ALERT", &TzUserEventChannel);
		}
		xTracePrint(TzUserEventChannel, cDfmPrintBuffer);
    }
#endif

	DFM_CFG_PRINT(LNBR "DFM Alert: ");
	DFM_CFG_PRINT(cDfmPrintBuffer);
	DFM_CFG_PRINT(LNBR);

	if (xDfmAlertBegin(alerttype, cDfmPrintBuffer, &xAlertHandle) == DFM_SUCCESS)
	{
		(void)xDfmKernelPortGetCurrentTaskName(&szCurrentTaskName);

#ifdef DFM_SYMPTOM_CURRENT_TASK
		xDfmAlertAddSymptom(xAlertHandle, DFM_SYMPTOM_CURRENT_TASK, ulDfmCalculateChecksum(szCurrentTaskName, 32));
#endif

#ifdef DFM_SYMPTOM_STACKPTR
		xDfmAlertAddSymptom(xAlertHandle, DFM_SYMPTOM_STACKPTR, pInfo->sp);
#endif

		if (dfmTrapInfo.alertType >= 0)
		{
			/* On DFM_TRAP */
#ifdef DFM_SYMPTOM_FILE
			xDfmAlertAddSymptom(xAlertHandle, DFM_SYMPTOM_FILE, ulDfmCalculateChecksum(szFileName, 32));
#endif

#ifdef DFM_SYMPTOM_LINE
			xDfmAlertAddSymptom(xAlertHandle, DFM_SYMPTOM_LINE, dfmTrapInfo.line);
#endif
		}
		else
		{
			/* On hard faults */
#ifdef DFM_SYMPTOM_CFSR
			xDfmAlertAddSymptom(xAlertHandle, DFM_SYMPTOM_CFSR, ARM_CORTEX_M_CFSR_REGISTER);
#endif
		}

#if ((DFM_CFG_CRASH_ADD_TRACE) >= 1)
		if (uiTraceWasEnabledAtDumpStart != 0U)
		{
		prvAddTracePayload();
		}
#endif

		DFM_CFG_PRINT("  DFM: Storing the alert." LNBR);
	}
	else
	{
		DFM_CFG_PRINT("  DFM: Not yet initialized. Alert ignored." LNBR); // Always log this!
	}
	DFM_CFG_PRINT(LNBR);

}

#if ((DFM_CFG_CRASH_ADD_TRACE) >= 1)
static void prvAddTracePayload(void)
{
	void* pvBuffer = (void*)0;
	uint32_t ulBufferSize = 0;	
    
	/* Register the event buffer as a payload. DumpEnd pauses the recorder
	 * before xDfmAlertEnd reads and saves the buffer. */
	xTraceGetEventBuffer(&pvBuffer, &ulBufferSize);
	xDfmAlertAddPayload(xAlertHandle, pvBuffer, ulBufferSize, "dfm_trace.psfs");
}
#endif

void CrashCatcher_DumpMemory(const void* pvMemory, CrashCatcherElementSizes elementSize, size_t elementCount)
{
	int32_t current_usage = (uint32_t)ucBufferPos - (uint32_t)ucDataBuffer;

	if ( current_usage + (elementSize*elementCount) >= CRASH_DUMP_BUFFER_SIZE)
	{
		ulCoredumpOverflowed = 1U;
		DFM_ERROR_PRINT(LNBR "DFM: Error, ucDataBuffer not large enough!" LNBR LNBR);
		return;
	}

	/* This function is called when CrashCatcher detects an internal stack overflow (it has a separate stack) */
	if (g_crashCatcherStack[0] != CRASH_CATCHER_STACK_SENTINEL)
	{
		/* Always try to print this error. But it might actually not print since the memory has been corrupted. */
		DFM_ERROR_PRINT("DFM: ERROR, stack overflow in CrashCatcher, see comment in dfmCrashCatcher.c" LNBR LNBR);

		/**********************************************************************************************************

		If you get here, there has been a stack overflow on the CrashCatcher stack.
		This is separate from the main stack and defined in CrashCatcher.c (g_crashCatcherStack).

		This error might happen because of diagnostic prints and other function calls while saving the alert.
		You may increase the stack size in CrashCatcherPriv.h or turn off the logging (DFM_CFG_USE_DEBUG_LOGGING).

		***********************************************************************************************************/

		// vDfmDisableInterrupts();
		for (;;); // Stop here...
	}

	if (elementCount == 0)
	{
		/* May happen if CRASH_MEM_REGION<X>_SIZE is set to 0 by mistake (e.g. if using 0 instead of 0xFFFFFFFF for CRASH_MEM_REGION<X>_START on unused slots. */
		DFM_ERROR_PRINT("DFM: Warning, memory region size is zero!" LNBR LNBR);
		return;
	}

	switch (elementSize)
	{

		case CRASH_CATCHER_BYTE:
			memcpy((void*)ucBufferPos, pvMemory, elementCount);
			ucBufferPos += elementCount;
			break;

		case CRASH_CATCHER_HALFWORD:
			dumpHalfWords(pvMemory, elementCount);
			break;

		case CRASH_CATCHER_WORD:
			dumpWords(pvMemory, elementCount);

			break;

		default:
			DFM_ERROR_PRINT(LNBR "DFM: Error, unhandled case!" LNBR LNBR);
			break;
	}
}

static void dumpHalfWords(const uint16_t* pMemory, size_t elementCount)
{
	size_t i;

	for (i = 0 ; i < elementCount ; i++)
	{
		uint16_t val = *pMemory++;
		memcpy((void*)ucBufferPos, &val, sizeof(val));

		ucBufferPos += sizeof(val);
	}
}

static void dumpWords(const uint32_t* pMemory, size_t elementCount)
{
	size_t i;
	for (i = 0 ; i < elementCount ; i++)
	{
		uint32_t val = *pMemory++;
		memcpy((void*)ucBufferPos, &val, sizeof(val));

		CC_DBG_LOG("%02d: %08X" LNBR, i, val);

		ucBufferPos += sizeof(val);
	}
}

CrashCatcherReturnCodes CrashCatcher_DumpEnd(void)
{
    CC_DBG_LOG("CrashCatcher_DumpEnd (DFM output begins)" LNBR);
    
#if ((DFM_CFG_CRASH_ADD_TRACE) >= 1)
	uint32_t uiRecorderNeedsResume = 0u;
#endif

	if (xAlertHandle != 0)
	{
		uint32_t size = (uint32_t)ucBufferPos - (uint32_t)ucDataBuffer;
		const char *szDumpName = dfmTrapInfo.alertType >= 0 ?
			DFM_TRAP_DUMP_NAME : DFM_FAULT_DUMP_NAME;

		if ((ulCoredumpOverflowed == 0U) &&
			(xDfmAlertAddPayload(xAlertHandle, ucDataBuffer, size,
				szDumpName) != DFM_SUCCESS))
		{
			DFM_ERROR_PRINT("DFM: Error, xDfmAlertAddPayload failed." LNBR);
		}

#if ((DFM_CFG_CRASH_ADD_TRACE) >= 1)
		/* Pause only while xDfmAlertEnd reads and saves the event buffer.
		 * Direct access avoids starting a new recorder session on resume. */
		if ((uiTraceWasEnabledAtDumpStart != 0U) &&
			xTraceIsRecorderEnabled())
		{
			pxTraceRecorderData->uiRecorderEnabled = 0u;
			uiRecorderNeedsResume = 1u;
		}
#endif

#ifdef DFM_CLOUD_PORT_ALWAYS_ATTEMPT_TRANSFER
		/* The cloud port has indicated it is always OK to attempt to transfer */
		if (xDfmAlertEnd(xAlertHandle) != DFM_SUCCESS)
		{
			DFM_CFG_PRINT("DFM: xDfmAlertEnd failed." LNBR);
		}

#else
		/* Cloud port transfer cannot be trusted, so we only attempt to store it */
		if (xDfmAlertEndOffline(xAlertHandle) != DFM_SUCCESS)
		{
			DFM_CFG_PRINT("DFM: xDfmAlertEndOffline failed." LNBR);
		}
#endif

	}

    
	CC_DBG_LOG("dfmTrapInfo.alertType: %d" LNBR, dfmTrapInfo.alertType);
	/* If triggered by DFM_TRAP */
	if (dfmTrapInfo.alertType != -1)
	{
		if (dfmTrapInfo.restart == 1)
		{
			CC_DBG_LOG("Type: DFM_TRAP with restart." LNBR);
			CRASH_FINALIZE();
		}
		else
		{
			CC_DBG_LOG("Type: DFM_TRAP, no restart." LNBR);
			
#if ((DFM_CFG_CRASH_ADD_TRACE) >= 1)
			if (uiRecorderNeedsResume != 0u)
			{
				pxTraceRecorderData->uiRecorderEnabled = 1u;
			}
#endif
		}

		vDfmCrashCatcherClearTrapInfo();
	}
	else
	{
		CC_DBG_LOG("Type: Fault Exception." LNBR);
		/* If triggered by hard fault or similar */
		CRASH_FINALIZE();
	}

	xAlertHandle = (void*)0;
#if ((DFM_CFG_CRASH_ADD_TRACE) >= 1)
	uiTraceWasEnabledAtDumpStart = 0U;
#endif
	return CRASH_CATCHER_EXIT;
}

/* Called by gcc stack-checking code when using the gcc option -fstack-protector-strong */
void __stack_chk_fail(void)
{
#ifdef DFM_TYPE_STACK_CHK_FAILED
	/* The stack has been corrupted by the previous function in the call stack (before __stack_chk_fail) */
	DFM_TRAP(DFM_TYPE_STACK_CHK_FAILED, "Stack corruption", 1);
#endif
        
    while(1); /* Avoids warnings in IAR (declared "noreturn").*/

}

#if ((defined (__ARM_ARCH_8M_MAIN__ ) && (__ARM_ARCH_8M_MAIN__ == 1)) || \
     (defined (__ARM_ARCH_8M_BASE__ ) && (__ARM_ARCH_8M_BASE__ == 1))    )

static volatile uint32_t dfm_stored_psplim = 0;
static volatile uint32_t dfm_stored_msplim = 0;

#define __STACK_IS_MSP 0
#define __STACK_IS_PSP 1
#define SPSEL_MASK (1UL << CONTROL_SPSEL_Pos)

static uint32_t prvGetCurrentStack(void)
{
    if (__get_IPSR() != 0U) 
    {
        /* Handler mode (exceptions) - always MSP */
        return __STACK_IS_MSP;
    }
    
    /* Thread mode - stack is given by SPSEL bit in CONTROL reg.*/
    if (__get_CONTROL() & SPSEL_MASK)
    {
        // SPSEL bit set -> PSP
        return __STACK_IS_PSP;
    }
    else
    {
        // SPSEL bit not set -> MSP
        return __STACK_IS_MSP;
    }
}

/* void dfmStackOverflowCheckSuspend(void)
 * Used in DFM_TRAP(). ArmV8-M processors like Cortex-M33 have stack overflow
 * detection in hardware. This must be suspended inside DFM_TRAP, since 
 * CrashCatcher updates the stack pointer to use a separate stack. (This is to
 * avoid stack overflows on the original stack. Ironically, this
 * stack switch can appear as a stack overflow to the processor and trigger a
 * fault exception. */

void dfmStackOverflowCheckSuspend(void)
{
    if (prvGetCurrentStack() == __STACK_IS_PSP)
    {
        // Save and clear process stack pointer limit
        dfm_stored_psplim = __get_PSPLIM();
        __set_PSPLIM(0);
        __ISB();
    }
    else
    {
        // Save and clear main stack pointer limit
        dfm_stored_msplim = __get_MSPLIM();
        __set_MSPLIM(0);    
        __ISB();
    }

}

// Used in DFM_TRAP(). Restores the current stack limit register after the core dump.
void dfmStackOverflowCheckResume(void)
{        
    if (prvGetCurrentStack() == __STACK_IS_PSP)
    {
        __set_PSPLIM(dfm_stored_psplim);
        __ISB();
    }
    else
    {
        __set_MSPLIM(dfm_stored_msplim);    
        __ISB();
    }       
}
#endif


#if (0)
void test_dfmCoreDump_with_known_regs()
{
    // Sets core regs to known values.
    __asm volatile (
        "ldr r0,  =0x0000A0A0 \n"
        "ldr r1,  =0x1111B1B1 \n"
        "ldr r2,  =0x2222C2C2 \n"
        "ldr r3,  =0x3333D3D3 \n"
        "ldr r4,  =0x4444E4E4 \n"
        "ldr r5,  =0x5555F5F5 \n"
        "ldr r6,  =0x6666A6A6 \n"
        "ldr r7,  =0x7777B7B7 \n"
        "ldr r8,  =0x8888C8C8 \n"
        "ldr r9,  =0x9999D9D9 \n"
        "ldr r10, =0xAAAAEAEA \n"
        "ldr r11, =0xBBBBFBFB \n"
        "ldr r12, =0xCCCCACAC \n"
        ::: "r0","r1","r2","r3","r4","r5","r6","r7","r8","r9","r10","r11","r12",
          "cc","memory"
	);
    
    __asm volatile ("nop");
    DFM_TRAP(DFM_TYPE_STACK_CHK_FAILED, "TEST ALERT", 0);
    __asm volatile ("nop");

}
#endif  


#endif
