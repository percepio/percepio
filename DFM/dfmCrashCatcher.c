/*
 * Percepio DFM v2.0.0
 * Copyright 2023 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * DFM Crash Catcher integration
 */

#include <CrashCatcher.h>
#include <string.h>
#include <dfm.h>
#include <dfmCrashCatcher.h>
#include <CrashCatcherPriv.h>
#include <dfmKernelPort.h>

#if ((DFM_CFG_ENABLED) >= 1)

#if ((DFM_CFG_CRASH_ADD_TRACE) >= 1)
#include <trcRecorder.h>
static void prvAddTracePayload();
#endif

/* See https://developer.arm.com/documentation/dui0552/a/cortex-m3-peripherals/system-control-block/configurable-fault-status-register*/
#define ARM_CORTEX_M_CFSR_REGISTER *(uint32_t*)0xE000ED28

static DfmAlertHandle_t xAlertHandle = 0;

dfmTrapInfo_t dfmTrapInfo = {-1, NULL, NULL, -1};

#if ((DFM_CFG_CRASH_ADD_TRACE) >= 1)
static TraceStringHandle_t TzUserEventChannel = NULL;
#endif

// TODO: Better to use a random number here, so it is harder to spoof?
void *__stack_chk_guard = (void *)0xdeadbeef;

static uint8_t* ucBufferPos;
static uint8_t ucDataBuffer[CRASH_DUMP_BUFFER_SIZE] __attribute__ ((aligned (8)));

static void dumpHalfWords(const uint16_t* pMemory, size_t elementCount);
static void dumpWords(const uint32_t* pMemory, size_t elementCount);

uint32_t stackPointer = 0;

/* Used for snprintf calls */
char cDfmPrintBuffer[128];

static char* prvGetFileNameFromPath(char* szPath)
{
	return strrchr(szPath, '/')+1; /* +1 to skip the last '/' character */
}

static uint32_t prvCalculateChecksum(char *ptr, size_t maxlen)
{
	uint32_t chksum = 0;
	int i = 0;

	if (ptr == NULL)
	{
		return 0;
	}

	while ((ptr[i] != (char)0) && (i < maxlen))
	{
		chksum += (uint32_t)ptr[i];
		i++;
	}

	return chksum;
}

const CrashCatcherMemoryRegion* CrashCatcher_GetMemoryRegions(void)
{
	static CrashCatcherMemoryRegion regions[] = {
			{0xFFFFFFFF, 0xFFFFFFFF, CRASH_CATCHER_BYTE},
			{CRASH_MEM_REGION1_START, CRASH_MEM_REGION1_START + CRASH_MEM_REGION1_SIZE, CRASH_CATCHER_BYTE},
			{CRASH_MEM_REGION2_START, CRASH_MEM_REGION2_START + CRASH_MEM_REGION2_SIZE, CRASH_CATCHER_BYTE},
			{CRASH_MEM_REGION3_START, CRASH_MEM_REGION3_START + CRASH_MEM_REGION3_SIZE, CRASH_CATCHER_BYTE}
	};

	/* Region 0 is reserved, always relative to the current stack pointer */
	regions[0].startAddress = stackPointer;
	regions[0].endAddress = stackPointer + CRASH_STACK_CAPTURE_SIZE;

	return regions;
}

void CrashCatcher_DumpStart(const CrashCatcherInfo* pInfo)
{
	int alerttype;
	char* szFileName = (void*)0;
	char* szCurrentTaskName = (void*)0;

	stackPointer = pInfo->sp;

#if ((DFM_CFG_CRASH_ADD_TRACE) >= 1)
	if (TzUserEventChannel == 0)
	{
		xTraceStringRegister("ALERT", &TzUserEventChannel);
	}
#endif

	ucBufferPos = &ucDataBuffer[0];

	DFM_DEBUG_PRINT("\nDFM Alert\n");

	if (dfmTrapInfo.alertType >= 0)
	{
		/* On the DFM_TRAP macro.
		 * This sets dfmTrapInfo and then generates an NMI exception to trigger this error handler.
		 * dfmTrapInfo.message = "Assert failed" or similar.
		 * dfmTrapInfo.file = __FILE__ (full path, extract the filename from this!)
		 * dfmTrapInfo.line = __LINE__ (integer)
		 * */
		szFileName = prvGetFileNameFromPath(dfmTrapInfo.file);
		snprintf(cDfmPrintBuffer, sizeof(cDfmPrintBuffer), "%s at %s:%u", dfmTrapInfo.message, szFileName, dfmTrapInfo.line);

		DFM_DEBUG_PRINT("  DFM_TRAP(): ");
		DFM_DEBUG_PRINT(cDfmPrintBuffer);
		DFM_DEBUG_PRINT("\n");

		alerttype = dfmTrapInfo.alertType;
	}
	else
	{
		DFM_DEBUG_PRINT("  DFM: Hard fault\n");

		snprintf(cDfmPrintBuffer, sizeof(cDfmPrintBuffer), "Hard fault exception (CFSR reg: 0x%08X)", (unsigned int)ARM_CORTEX_M_CFSR_REGISTER);

		alerttype = DFM_TYPE_HARDFAULT;
	}

	if (xDfmAlertBegin(alerttype, cDfmPrintBuffer, &xAlertHandle) == DFM_SUCCESS)
	{
		(void)xDfmKernelPortGetCurrentTaskName(&szCurrentTaskName);

		xDfmAlertAddSymptom(xAlertHandle, DFM_SYMPTOM_CURRENT_TASK, prvCalculateChecksum(szCurrentTaskName, 32));
		xDfmAlertAddSymptom(xAlertHandle, DFM_SYMPTOM_STACKPTR, pInfo->sp);

		if (dfmTrapInfo.alertType >= 0)
		{
			/* On DFM_TRAP */
			xDfmAlertAddSymptom(xAlertHandle, DFM_SYMPTOM_FILE, prvCalculateChecksum(szFileName, 32));
			xDfmAlertAddSymptom(xAlertHandle, DFM_SYMPTOM_LINE, dfmTrapInfo.line);
		}
		else
		{
			/* On hard faults */
			xDfmAlertAddSymptom(xAlertHandle, DFM_SYMPTOM_ARM_SCB_FCSR, ARM_CORTEX_M_CFSR_REGISTER);

			/******************************************************************
			 * TODO: Add MMAR and BFAR regs here as symptoms to get the address
			 * of the problematic instruction. Would be a good symptom.
			 *****************************************************************/
		}

#if ((DFM_CFG_CRASH_ADD_TRACE) >= 1)
		xTracePrint(TzUserEventChannel, cDfmPrintBuffer);
		prvAddTracePayload();
#endif
		DFM_DEBUG_PRINT("  DFM: Storing the alert.\n");
	}
	else
	{
		DFM_DEBUG_PRINT("  DFM: Not yet initialized. Alert ignored.\n"); // Always log this!
	}
	DFM_DEBUG_PRINT("\n");

	dfmTrapInfo.alertType = -1;
	dfmTrapInfo.message = NULL;
}

#if ((DFM_CFG_CRASH_ADD_TRACE) >= 1)
static void prvAddTracePayload()
{
	char* szName;
	void* pvBuffer = (void*)0;
	uint32_t ulBufferSize = 0;
	if (TRC_CFG_RECORDER_MODE == TRC_RECORDER_MODE_STREAMING)
	{
		szName = "dfm_trace.psfs";
	}
	else if (TRC_CFG_RECORDER_MODE == TRC_RECORDER_MODE_SNAPSHOT)
	{
		szName = "dfm_trace.trc";
	}

	if (xTraceIsRecorderEnabled() == 1)
	{
		xTraceDisable();
	}
	xTraceGetEventBuffer(&pvBuffer, &ulBufferSize);
	xDfmAlertAddPayload(xAlertHandle, pvBuffer, ulBufferSize, szName);
}
#endif

void CrashCatcher_DumpMemory(const void* pvMemory, CrashCatcherElementSizes elementSize, size_t elementCount)
{
	int32_t current_usage = (uint32_t)ucBufferPos - (uint32_t)ucDataBuffer;

	/* This function is called when CrashCatcher detects an internal stack overflow (it has a separate stack) */
	if (g_crashCatcherStack[0] != CRASH_CATCHER_STACK_SENTINEL)
	{
		/* Always try to print this error. But it might actually not print since the memory has been corrupted. */
		DFM_ERROR_PRINT("DFM: ERROR, stack overflow in CrashCatcher, see comment in dfmCrashCatcher.c\n");

		/**********************************************************************************************************

		If you get here, there has been a stack overflow on the CrashCatcher stack.
		This is separate from the main stack and defined in CrashCatcher.c (g_crashCatcherStack).

		This error might happen because of diagnostic prints and other function calls while saving the alert.
		You may increase the stack size in CrashCatcherPriv.h or turn off the logging (DFM_CFG_USE_DEBUG_LOGGING).

		***********************************************************************************************************/

		vDfmDisableInterrupts();
		for (;;); // Stop here...
	}

	if (elementCount == 0)
	{
		/* May happen if CRASH_MEM_REGION<X>_SIZE is set to 0 by mistake (e.g. if using 0 instead of 0xFFFFFFFF for CRASH_MEM_REGION<X>_START on unused slots. */
		DFM_ERROR_PRINT("DFM: Warning, memory region size is zero!\n");
		return;
	}

	switch (elementSize)
	{

		case CRASH_CATCHER_BYTE:

			if ( current_usage + elementCount >= CRASH_DUMP_BUFFER_SIZE)
			{
				DFM_ERROR_PRINT("\nDFM: Error, ucDataBuffer not large enough!\n\n");
				return;
			}

			memcpy((void*)ucBufferPos, pvMemory, elementCount);
			ucBufferPos += elementCount;
			break;

		case CRASH_CATCHER_HALFWORD:

			if ( current_usage + elementCount*2 >= CRASH_DUMP_BUFFER_SIZE)
			{
				DFM_ERROR_PRINT("\nDFM: Error, ucDataBuffer not large enough!\n\n");
				return;
			}
			dumpHalfWords(pvMemory, elementCount);

			break;

		case CRASH_CATCHER_WORD:

			if ( current_usage + elementCount*4 >= CRASH_DUMP_BUFFER_SIZE)
			{
				DFM_ERROR_PRINT("\nDFM: Error, ucDataBuffer not large enough!\n\n");
				return;
			}

			dumpWords(pvMemory, elementCount);

			break;

		default:
			DFM_ERROR_PRINT("\nDFM: Error, unhandled case!\n\n");
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
		ucBufferPos += sizeof(val);
	}
}

CrashCatcherReturnCodes CrashCatcher_DumpEnd(void)
{
	if (xAlertHandle != 0)
	{
		uint32_t size = (uint32_t)ucBufferPos - (uint32_t)ucDataBuffer;
		if (xDfmAlertAddPayload(xAlertHandle, ucDataBuffer, size, CRASH_DUMP_NAME) == DFM_SUCCESS)
		{
			DFM_DEBUG_PRINT("  DFM: Crash dump stored.\n");
		}
		else
		{
			DFM_ERROR_PRINT("  DFM: Failed storing crash dump.\n");
		}

#ifdef DFM_CLOUD_PORT_ALWAYS_ATTEMPT_TRANSFER
		/* The cloud port has indicated it is always OK to attempt to transfer */
		if (xDfmAlertEnd(xAlertHandle) == DFM_SUCCESS)
#else
		/* Cloud port transfer cannot be trusted, so we only attempt to store it */
		if (xDfmAlertEndOffline(xAlertHandle) == DFM_SUCCESS)
#endif
		{
			DFM_DEBUG_PRINT("  DFM: Alert stored OK.\n");
		}
		else
		{
			DFM_ERROR_PRINT("  DFM: Failed storing alert.\n");
		}

	}

	CRASH_FINALIZE();
	return CRASH_CATCHER_EXIT;
}


/* Called by gcc stack-checking code when using the gcc option -fstack-protector-strong */
void __stack_chk_fail(void)
{
	// If this happens, the stack has been corrupted by the previous function in the call stack.
	// Note that the exact location of the stack corruption is not known, since detected when exiting the function.
	DFM_TRAP(DFM_TYPE_STACK_CHK_FAILED, "Stack corruption detected");
}

#endif
