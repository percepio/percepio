/*
 * Percepio DFM
 * Copyright 2023-2026 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr Kernel port
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/arch/exception.h>
#include <kernel_arch_data.h>

#include <dfm.h>

#if defined(CONFIG_PERCEPIO_DFM_CFG_ENABLE_COREDUMPS)
#include <zephyr/debug/coredump.h>
#include <zephyr/sys/byteorder.h>
#endif

/* Scratch related includes */
#include <zephyr/storage/flash_map.h>
#include <dfmConfig.h>
#include <dfmUtility.h>

#if ((DFM_CFG_ENABLED) == 1)

#if (CONFIG_PERCEPIO_DFM_CFG_ADD_TRACE >= 1)
static TraceStringHandle_t TzUserEventChannel = 0;
#endif

#if defined(CONFIG_PERCEPIO_DFM_CFG_COREDUMP_RETAIN) && !defined(CONFIG_PERCEPIO_DFM_CFG_RETAINED_MEMORY)
#error "DFM is configured to store Core Dumps in Retained Memory but that isn't enabled in DFM."
#endif

#define MAX_COREDUMP_PARTS 8

/* Alert with (optional) trace (if CONFIG_PERCEPIO_DFM_CFG_ADD_TRACE=y) */
void prvDfmTrap_NoCoreDump(int alertType, const char *message, const char *file, int line, int restart);

dfmTrapInfo_t dfmTrapInfo = {0};

DfmKernelPortData_t* pxKernelPortData;

#if defined(CONFIG_PERCEPIO_DFM_CFG_INITIALIZE_FOR_LOCAL_USE)
static int dfm_application_init(void)
{
	xDfmInitializeForLocalUse();

	return 0;
}

/* Specify DFM module initialization stages */
SYS_INIT(dfm_application_init, APPLICATION, 0);
#endif /* PERCEPIO_DFM_CFG_INITIALIZE_FOR_LOCAL_USE */

DfmResult_t xDfmKernelPortInitialize(DfmKernelPortData_t *pxBuffer)
{
	if (pxBuffer == (void*)0)
	{
		return DFM_FAIL;
	}

	pxKernelPortData = pxBuffer;

	return DFM_SUCCESS;
}

DfmResult_t xDfmKernelPortGetCurrentTaskName(const char** pszTaskName)
{
#ifdef CONFIG_THREAD_NAME
	k_tid_t current_thread = k_current_get();
	DfmResult_t ret;
	if (current_thread == (void*)0) {
		current_thread = k_sched_current_thread_query();
	}
	if (current_thread == (void*)0) {
		ret = DFM_FAIL;
	} else {
		*pszTaskName = k_thread_name_get(current_thread);
		ret = DFM_SUCCESS;
}
	return ret;
#else
	return DFM_FAIL;
#endif
}

/*
 * Functions for the coredump backend api, which in turn will use the dfmBackend for storing data
 * TODO: We probably don't want this in the kernel port but rather in its own file, like with crashcatcher.
 */

enum eDfmCoredumpState {
	DFM_COREDUMP_STATE_STARTED,
	DFM_COREDUMP_STATE_HEADER,
	DFM_COREDUMP_STATE_DATA
};

#if defined(CONFIG_PERCEPIO_DFM_CFG_ENABLE_COREDUMPS)

typedef struct DfmCoredumpPayload {
	union {
		char id;
		struct coredump_hdr_t hdr;
		struct coredump_arch_hdr_t arch_hdr;
		struct coredump_threads_meta_hdr_t threads_meta_hdr;
		struct coredump_mem_hdr_t mem_hdr;
	} pubHeaderBuffer;
	size_t ulHeaderSize;
	void* pxContent;
	size_t ulContentSize;
} DfmCoredumpPayload_t;

static enum eDfmCoredumpState eCoreDumpState = DFM_COREDUMP_STATE_STARTED;
static DfmCoredumpPayload_t pxDfmCoredumpParts[MAX_COREDUMP_PARTS]; // The zephyr coredump process won't add more than 3 headers
static uint8_t ubDfmCoreDumpHeaderCounter = 0;
static uint8_t ubDfmPayloadBuffer[CONFIG_PERCEPIO_DFM_CFG_MAX_COREDUMP_SIZE];
static uint32_t ulDfmPayloadBufferBytesUsed = 0;
static int iDfmError;

DfmResult_t xDfmAlertAddCoredump(DfmAlertHandle_t xAlertHandle, const char* szPayloadName)
{
	if (ubDfmCoreDumpHeaderCounter < 1)
		return DFM_FAIL;

	ulDfmPayloadBufferBytesUsed = 0;
	memset(ubDfmPayloadBuffer, 0, sizeof(ubDfmPayloadBuffer));
	uint8_t* pubDfmPayloadBufferPosition = ubDfmPayloadBuffer;

	for (int i=0; i<ubDfmCoreDumpHeaderCounter; i++)
	{
		DfmCoredumpPayload_t* pxCurrentPayload = &pxDfmCoredumpParts[i];

		size_t ulContentSize = pxCurrentPayload->ulContentSize;

		/* Initial size check to avoid overflowing the buffer, i.e. does this part of the chunk fit within the chunk */
		if (ulDfmPayloadBufferBytesUsed + pxCurrentPayload->ulHeaderSize + ulContentSize > CONFIG_PERCEPIO_DFM_CFG_MAX_COREDUMP_SIZE)
			return DFM_FAIL;

		/* Copy the header */
		memcpy(pubDfmPayloadBufferPosition, &pxCurrentPayload->pubHeaderBuffer, pxCurrentPayload->ulHeaderSize);
		pubDfmPayloadBufferPosition += pxCurrentPayload->ulHeaderSize;
		ulDfmPayloadBufferBytesUsed += pxCurrentPayload->ulHeaderSize;

		if (pxCurrentPayload->pxContent != (void*)0)
		{
			/* Copy the content */
			memcpy(pubDfmPayloadBufferPosition, pxCurrentPayload->pxContent, ulContentSize);
			pubDfmPayloadBufferPosition += ulContentSize;
			ulDfmPayloadBufferBytesUsed += ulContentSize;
		}
	}

	DfmResult_t xResult;
	xResult = xDfmAlertAddPayload(
		xAlertHandle,
		ubDfmPayloadBuffer,
		ulDfmPayloadBufferBytesUsed,
		szPayloadName
	);

	return xResult;
}

/**
 * Start a new coredump, will wipe the memory area and reset the internal counters used by this kernel port when a coredump
 * is saved.
 * This function is called from the Zephyr kernel.
 */
static void xDfmCoredumpBackendStart(void)
{
	eCoreDumpState = DFM_COREDUMP_STATE_STARTED;
	ubDfmCoreDumpHeaderCounter = 0;
	memset(pxDfmCoredumpParts, 0, sizeof(pxDfmCoredumpParts));
	iDfmError = 0;
}

/**
 * Internally store a part of a coredump within the internal array of DfmCoreDumpPayloads.
 * This function is called from the Zephyr kernel.
 * @param pxBuffer
 * @param ulBufferLength
 */
static void xDfmCoredumpBackendBufferOutput(uint8_t* pxBuffer, size_t ulBufferLength)
{
	/*
	 * To avoid nasty buffer overflows, stop dumping any more data in case the maximum
	 * amount of payloads has been reached. Since the signature of the function expected by Zephyr
	 * for outputting backend data is a void, error handling can unfortunately not be implemented here.
	 */
	if (ubDfmCoreDumpHeaderCounter >= MAX_COREDUMP_PARTS)
		return;

	DfmCoredumpPayload_t* pxCurrentPayload = &pxDfmCoredumpParts[ubDfmCoreDumpHeaderCounter];

	switch (eCoreDumpState)
	{

		case DFM_COREDUMP_STATE_STARTED:
		{
			pxCurrentPayload->ulHeaderSize = ulBufferLength;
			memcpy(&pxCurrentPayload->pubHeaderBuffer, pxBuffer, ulBufferLength);
			ubDfmCoreDumpHeaderCounter++;
			eCoreDumpState = DFM_COREDUMP_STATE_HEADER;
			break;
		}

		case DFM_COREDUMP_STATE_HEADER:
		{
			pxCurrentPayload->ulHeaderSize = ulBufferLength;
			memcpy(&pxCurrentPayload->pubHeaderBuffer, pxBuffer, ulBufferLength);
			eCoreDumpState = DFM_COREDUMP_STATE_DATA;
			break;
		}

		case DFM_COREDUMP_STATE_DATA: {
			pxCurrentPayload->ulContentSize = ulBufferLength;
			pxCurrentPayload->pxContent = pxBuffer;
			ubDfmCoreDumpHeaderCounter++;
			eCoreDumpState = DFM_COREDUMP_STATE_HEADER;
			break;
		}

	}

}


#include <zephyr/fatal_types.h>

const char *zephyr_reason_to_str(unsigned int reason)
{
    switch (reason) {
    case K_ERR_CPU_EXCEPTION:   return "Fatal error: CPU exception";
    case K_ERR_KERNEL_OOPS:     return "Fatal error: Kernel oops";
    case K_ERR_KERNEL_PANIC:    return "Fatal error: Kernel panic";
    case K_ERR_STACK_CHK_FAIL:  return "Fatal error: Stack check fail";
    case K_ERR_SPURIOUS_IRQ:    return "Fatal error: Spurious IRQ";    
    default:      
		snprintf(cDfmPrintBuffer, sizeof(cDfmPrintBuffer), "Fatal error: Other (reason code: %u)", reason);
		return cDfmPrintBuffer;
    }
}

static int zephyr_reason_to_alert_type(unsigned int reason)
{
	if (reason == K_ERR_STACK_CHK_FAIL)
	{
		return DFM_TYPE_STACK_CHK_FAILED;
	}

#if defined(CONFIG_CPU_CORTEX_M)
	/* DFM has one common alert type for Cortex-M fault exceptions. */
	if ((reason == K_ERR_CPU_EXCEPTION) ||
		((reason >= K_ERR_ARM_MEM_GENERIC) &&
		 (reason <= K_ERR_ARM_SECURE_LAZY_STATE_ERROR)))
	{
		return DFM_TYPE_HARDFAULT;
	}
#endif

	return DFM_TYPE_ZEPHYR_FATAL_ERROR;
}

/**
 * This function is called from the the Zephyr kernel.
 */
static void xDfmCoredumpBackendEnd(void)
{
	DfmAlertHandle_t xAlertHandle;
	const char *szFileName = (void*)0;
#ifdef DFM_SYMPTOM_CURRENT_TASK
	const char *szCurrentTaskName = (void*)0;
#endif
#if defined(CONFIG_PERCEPIO_DFM_CFG_ADD_TRACE)
	uint32_t uiRecorderNeedsResume = 0u;
#endif

	/* Examine the header to see whether this was a coredump created by the user or triggered from Zephyr */
	struct coredump_hdr_t* pxCoredumpHeader = &pxDfmCoredumpParts[0].pubHeaderBuffer.hdr;
	const unsigned int reason = sys_le16_to_cpu(pxCoredumpHeader->reason);
	const bool isDfmTrap = (reason == K_ERR_DFM_TRAP);

	int alertType;
	const char *message;
	if (!isDfmTrap)
	{
		alertType = zephyr_reason_to_alert_type(reason);
		message = zephyr_reason_to_str(reason);
	}
	else
	{
		alertType = dfmTrapInfo.alertType;
		szFileName = szDfmGetFileNameFromPath(dfmTrapInfo.file);
		snprintf(cDfmPrintBuffer, sizeof(cDfmPrintBuffer), "%s at %s:%u", dfmTrapInfo.message, szFileName, dfmTrapInfo.line);
		message = cDfmPrintBuffer;
	}

	if (xDfmAlertBegin(alertType, message, &xAlertHandle) == DFM_SUCCESS)
	{
		/* TODO: Look into how to add various symptoms caught by the coredump here */
		if (!isDfmTrap)
		{
			// A fault/fatal error, from the Zephyr fault handling
			xDfmAlertAddCoredump(xAlertHandle, "fault.zpr");
		}
		else
		{
			/* Add the same DFM_TRAP symptoms as the CrashCatcher port. */
#ifdef DFM_SYMPTOM_CURRENT_TASK
			(void)xDfmKernelPortGetCurrentTaskName(&szCurrentTaskName);
			xDfmAlertAddSymptom(xAlertHandle, DFM_SYMPTOM_CURRENT_TASK,
				ulDfmCalculateChecksum(szCurrentTaskName, 32));
#endif

#ifdef DFM_SYMPTOM_STACKPTR
#if defined(CONFIG_CPU_CORTEX_M)
			xDfmAlertAddSymptom(xAlertHandle, DFM_SYMPTOM_STACKPTR,
				z_arm_coredump_fault_sp);
#endif
#endif

#ifdef DFM_SYMPTOM_FILE
			xDfmAlertAddSymptom(xAlertHandle, DFM_SYMPTOM_FILE,
				ulDfmCalculateChecksum(szFileName, 32));
#endif

#ifdef DFM_SYMPTOM_LINE
			xDfmAlertAddSymptom(xAlertHandle, DFM_SYMPTOM_LINE,
				dfmTrapInfo.line);
#endif

			// A DFM_TRAP call. Use a different payload name, to allow for alternative gdb script (simplified view).
			xDfmAlertAddCoredump(xAlertHandle, "trap.zpr");
		}

		/* Add the reason code as a symptom */
		if (!isDfmTrap)
		{
			xDfmAlertAddSymptom(xAlertHandle, DFM_SYMPTOM_ZEPHYR_FATAL_ERROR_REASON, reason);
		}

#if defined(CONFIG_PERCEPIO_DFM_CFG_ADD_TRACE)
		if (xTraceIsRecorderEnabled())
		{
		if (TzUserEventChannel == 0)
		{
			xTraceStringRegister("ALERT", &TzUserEventChannel);
		}
		xTracePrint(TzUserEventChannel, message);
		xDfmAlertAddTrace(xAlertHandle);
			uiRecorderNeedsResume = 1u;
		}
#endif

#if defined(CONFIG_PERCEPIO_DFM_CFG_COREDUMP_RETAIN)
		xDfmAlertEndCustom(xAlertHandle, DFM_ALERT_END_TYPE_RETAIN);
#elif defined(CONFIG_PERCEPIO_DFM_CFG_COREDUMP_STORE)
		xDfmAlertEndCustom(xAlertHandle, DFM_ALERT_END_TYPE_STORE);
#elif defined(CONFIG_PERCEPIO_DFM_CFG_COREDUMP_SEND)
	xDfmAlertEndCustom(xAlertHandle, DFM_ALERT_END_TYPE_SEND);
#else
		xDfmAlertEnd();
#endif
	}

	/* DFM_TRAP metadata is valid for one coredump only. Leaving it populated
	 * would make a later Zephyr fatal error look like the previous trap. */
	if (isDfmTrap)
	{
		if (dfmTrapInfo.restart == 1)
		{
#if defined(CONFIG_REBOOT)					
			DFM_CFG_PRINT("DFM: Restart requested by DFM_TRAP.\n");
    		k_busy_wait(1000);   /* Let final UART chars drain before rebooting. */
		    sys_reboot(SYS_REBOOT_COLD);
#else
			DFM_CFG_PRINT("DFM: Restart requested by DFM_TRAP, but CONFIG_REBOOT not enabled.\n");
#endif		
		}

		/* A successful reboot does not return. If execution continues, resume
		 * the same recorder session that this alert paused. */
#if defined(CONFIG_PERCEPIO_DFM_CFG_ADD_TRACE)
		if (uiRecorderNeedsResume != 0u)
		{
			pxTraceRecorderData->uiRecorderEnabled = 1u;
		}
#endif

		memset(&dfmTrapInfo, 0, sizeof(dfmTrapInfo));
	}

}


static int xDfmCoredumpBackendCmd(enum coredump_cmd_id eCmdId, void *arg)
{
	int ret;

	switch (eCmdId) {
	case COREDUMP_CMD_CLEAR_ERROR:
		ret = 0;
		iDfmError = 0;
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static int xDfmCoredumpBackendQuery(enum coredump_query_id eQueryId, void *arg)
{
	int ret;

	switch (eQueryId) {
	case COREDUMP_QUERY_GET_ERROR:
		ret = iDfmError;
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

struct coredump_backend_api coredump_backend_other = {
	.start = xDfmCoredumpBackendStart,
	.end = xDfmCoredumpBackendEnd,
	.buffer_output = xDfmCoredumpBackendBufferOutput,
	.cmd = xDfmCoredumpBackendCmd,
	.query = xDfmCoredumpBackendQuery
};
#endif

#if defined(CONFIG_PERCEPIO_TRACERECORDER) && defined(CONFIG_PERCEPIO_TRC_CFG_STREAM_PORT_RINGBUFFER)
DfmResult_t xDfmAlertAddTrace(DfmAlertHandle_t xAlertHandle)
{
	void* pvBuffer = (void*)0;
	uint32_t ulBufferSize = 0;

	if (!xTraceIsRecorderEnabled())
	{
		return DFM_FAIL;
	}

	/* Pause without ending the recorder session. xTraceEnable() cannot be used
	 * to resume here because it starts a new session and clears the ring buffer. */
	pxTraceRecorderData->uiRecorderEnabled = 0u;

	if (xTraceGetEventBuffer(&pvBuffer, &ulBufferSize) != DFM_SUCCESS)
	{
		return DFM_FAIL;
	}

	if (xDfmAlertAddPayload(xAlertHandle, pvBuffer, ulBufferSize, "dfm_trace.psfs") != DFM_SUCCESS)
	{
		return DFM_FAIL;
	}

	return DFM_SUCCESS;
}
#endif

/* When DFM coredump is not supported */
void prvDfmTrap_NoCoreDump(int alertType, const char *message, const char *file, int line, int restart)
{
	DfmAlertHandle_t xAlertHandle;
#if defined(CONFIG_PERCEPIO_DFM_CFG_ADD_TRACE)
	uint32_t uiRecorderNeedsResume = 0u;
#endif

	/* DFM_TRAP must be a silent no-op until every DFM subsystem is ready. In
	 * particular, do not reach the restart path during early Zephyr init. */
	if (ulDfmIsInitialized() == 0U)
	{
		return;
	}
	
	const char* szFileName = szDfmGetFileNameFromPath(file);
	snprintf(cDfmPrintBuffer, sizeof(cDfmPrintBuffer), "%s at %s:%u", message, szFileName, line);

	if (xDfmAlertBegin(alertType, cDfmPrintBuffer, &xAlertHandle) == DFM_SUCCESS)
	{
		(void)xDfmAddFileAndLineSymptoms(xAlertHandle, szFileName, line);

		#if defined(CONFIG_PERCEPIO_DFM_CFG_ADD_TRACE)	
		if (xTraceIsRecorderEnabled())
		{
		if (TzUserEventChannel == 0)
		{
			xTraceStringRegister("ALERT", &TzUserEventChannel);
		}
		xTracePrint(TzUserEventChannel, cDfmPrintBuffer);
		xDfmAlertAddTrace(xAlertHandle);
			uiRecorderNeedsResume = 1u;
		}
#endif

		#if defined(CONFIG_PERCEPIO_DFM_CFG_COREDUMP_RETAIN)
		xDfmAlertEndCustom(xAlertHandle, DFM_ALERT_END_TYPE_RETAIN);
		#elif defined(CONFIG_PERCEPIO_DFM_CFG_COREDUMP_STORE)
		xDfmAlertEndCustom(xAlertHandle, DFM_ALERT_END_TYPE_STORE);
		#elif defined(CONFIG_PERCEPIO_DFM_CFG_COREDUMP_SEND)
		xDfmAlertEndCustom(xAlertHandle, DFM_ALERT_END_TYPE_SEND);
		#else
		xDfmAlertEnd(xAlertHandle);
#endif
	}

	if (restart == 1)
	{
#if defined(CONFIG_REBOOT)		
		DFM_CFG_PRINT("DFM: Restart requested by DFM_TRAP.\n");
    	k_busy_wait(1000);   /* Let final UART chars drain before rebooting. */
	    sys_reboot(SYS_REBOOT_COLD);
#else
		DFM_CFG_PRINT("DFM: Restart requested by DFM_TRAP, but CONFIG_REBOOT not enabled.\n");
#endif		
}

#if defined(CONFIG_PERCEPIO_DFM_CFG_ADD_TRACE)
	/* A successful reboot does not return. If execution continues, resume
	 * the same recorder session that this alert paused. */
	if (uiRecorderNeedsResume != 0u)
	{
		pxTraceRecorderData->uiRecorderEnabled = 1u;
	}
#endif
}

#if defined(CONFIG_PERCEPIO_DFM_CFG_ENABLE_COREDUMPS) && \
	defined(CONFIG_IRQ_OFFLOAD) && defined(CONFIG_CPU_CORTEX_M)

#include <cmsis_core.h>

static inline int in_msp_context(void)
{
    /* If handler mode (exception/ISR) -> always MSP stack*/
    if (__get_IPSR() != 0U) {
        return 1;
    }

    /* In thread mode -> CONTROL.SPSEL bit tells is MSP or PSP stack is used. */
    return ((__get_CONTROL() & CONTROL_SPSEL_Msk) == 0U) ? 1 : 0;
}

#define DFM_COREDUMP_DEBUG 0

static struct arch_esf regsdump;

/*
 * Register state saved by prvDfmTriggerCoredump() at a controlled SVC
 * boundary. Cortex-M exception entry stacks r0-r3, r12, lr, pc and xPSR in
 * hardware, but it does not stack the callee-saved registers r4-r11.
 *
 * Saving r4-r11 explicitly is required for stable stack unwinding of
 * unoptimized (-O0) code. In particular, the compiler commonly uses r7 as
 * the frame pointer at -O0, so an unavailable or modified r7 can prevent GDB
 * from following the interrupted call chain reliably.
 *
 * The capture is implemented as a naked assembly function so no compiler-
 * generated prologue, epilogue or temporary register allocation can change
 * r4-r11 before they are saved. The shim uses r0-r2 as scratch registers, so
 * the hardware exception frame contains those scratch values. The original
 * caller-saved values are captured separately before the DFM_TRAP metadata is
 * evaluated and copied into the synthetic exception frame by
 * prvDfmRunCoreDump().
 *
 * The objects below are written directly by the inline assembly. volatile
 * tells the compiler that their values can change outside normal C code, and
 * __used ensures that their symbols are emitted for the assembly references.
 * dfmTrapCallerSavedContext is the shared object declared in dfmUtility.h.
 */
volatile DfmTrapCallerSavedContext_t dfmTrapCallerSavedContext __used;
static volatile _callee_saved_t dfmTrapCalleeSavedContext __used;
static volatile uint32_t dfmTrapMspBeforeSvc __used;

#define DFM_STRINGIFY_(value) #value
#define DFM_STRINGIFY(value) DFM_STRINGIFY_(value)

/*
 * Capture r0-r3 and r12 before DFM_TRAP evaluates or stores its metadata.
 * Keep this function naked and limited to one basic assembly statement so no
 * compiler-generated prologue, epilogue, stack access or register allocation
 * can occur during the snapshot. The 16-byte push keeps SP 8-byte aligned, and
 * both SP and the temporary low-register changes are restored before returning
 * to compiler-generated code.
 */
void __attribute__((naked, noinline)) dfmTrapCaptureCallerSavedContext(void)
{
	__asm volatile (
		/* Preserve r0-r3 temporarily while r0 and r1 address and populate:
		 *   dfmTrapCallerSavedContext = {r0, r1, r2, r3, r12};
		 */
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

/*
 * Keep this function stackless. Its SVC exception frame is the top frame in
 * trap.zpr, while the hardware-stacked LR leads GDB directly back to
 * prvDfmTrap(). Do not add C statements to this naked function.
 *
 * This deliberately uses one Armv6-M-compatible instruction sequence on all
 * Cortex-M variants. Mainline variants could use a Thumb-2 STM to store
 * r4-r11 in one instruction, but that small optimization is insignificant
 * compared with creating and storing the coredump and would require another
 * architecture-specific path to maintain and test.
 *
 * The Armv6-M/Thumb-1 STM register list can only contain low registers
 * (r0-r7). Consequently, r4-r7 are stored together, while r8-r11 are copied
 * through the low scratch register r2 and stored individually. This sequence
 * is valid for both Baseline and Mainline Cortex-M variants.
 */
static __attribute__((naked, noinline)) void prvDfmTriggerCoredump(void)
{
	__asm volatile (
		/* Conceptually:
		 *   context.v1...v4 = r4...r7;
		 * STMIA writeback advances r0 to &context.v5.
		 */
		"ldr r0, =dfmTrapCalleeSavedContext\n"
		"stmia r0!, {r4-r7}\n"
		/* Thumb-1 cannot store high registers directly:
		 *   context.v5...v8 = r8...r11;
		 */
		"mov r2, r8\n"
		"str r2, [r0, #0]\n"
		"mov r2, r9\n"
		"str r2, [r0, #4]\n"
		"mov r2, r10\n"
		"str r2, [r0, #8]\n"
		"mov r2, r11\n"
		"str r2, [r0, #12]\n"
		/* context.psp is assigned the post-SVC exception-frame address
		 * by prvDfmRunCoreDump(), where that address is available.
		 */
		/* dfmTrapMspBeforeSvc = current MSP; */
		"ldr r0, =dfmTrapMspBeforeSvc\n"
		"mrs r1, MSP\n"
		"str r1, [r0]\n"
		/* Raise Zephyr's IRQ-offload SVC, then return to the C trap handler. */
		"svc #" DFM_STRINGIFY(_SVC_CALL_IRQ_OFFLOAD) "\n"
		"bx lr\n"
	);
}

static void prvDfmRunCoreDump(const void *parameter)
{
	ARG_UNUSED(parameter);
	
	const uint32_t* exc_frame = (const uint32_t *)(uintptr_t)__get_PSP();
	const uint32_t* msp_before_svc =
		(const uint32_t *)(uintptr_t)dfmTrapMspBeforeSvc;

	/* DFM_TRAP with coredumps assumes Arm Cortex-M
	   This solution relies on the specifics of the Arm Cortex-M SVC handler, 
	   where the first push to the MSP stack is the LR register containing
	   the EXC_RETURN code. This is found at the word before *msp_before_svc.
 	*/
	const uint32_t exc_return = *(msp_before_svc-1);

	if ((exc_return & 0xFFFFFF00U) != 0xFFFFFF00U)
	{
		#if (DFM_COREDUMP_DEBUG == 1)
		printk("prvDfmRunCoreDump: Sanity check of exc_return value failed. Coredump aborted. ")
		#endif

		return;
	}

	#if (DFM_COREDUMP_DEBUG == 1)
	printk("\nException frame at %p:\n\n", exc_frame);		
	printk(" R0:   0x%08X\n", dfmTrapCallerSavedContext.r0);
	printk(" R1:   0x%08X\n", dfmTrapCallerSavedContext.r1);
	printk(" R2:   0x%08X\n", dfmTrapCallerSavedContext.r2);
	printk(" R3:   0x%08X\n", dfmTrapCallerSavedContext.r3);
	printk(" R12:  0x%08X\n", dfmTrapCallerSavedContext.r12);
	printk(" LR:   0x%08X\n", exc_frame[5]);
	printk(" PC:   0x%08X\n", exc_frame[6]);
	printk(" xPSR: 0x%08X\n", exc_frame[7]);
	printk("\nStacked LR/EXC_RETURN: 0x%08x\n", exc_return);	
	#endif

	/* The arch_esf object (regsdump) is used by arch_coredump_info_dump() to read the register values 
	during the coredump() call. */
	memset(&regsdump, 0, sizeof(regsdump));
	
	/* First copy the exception frame that is automatically stacked by SVC. */
	memcpy(&regsdump.basic, exc_frame, sizeof(regsdump.basic));
	
	/* The assembly shim uses r0-r2 as scratch registers before SVC. Replace
	 * those values, together with r3 and r12, with the caller state captured
	 * at the start of DFM_TRAP, before its metadata was evaluated. */
	regsdump.basic.r0 = dfmTrapCallerSavedContext.r0;
	regsdump.basic.r1 = dfmTrapCallerSavedContext.r1;
	regsdump.basic.r2 = dfmTrapCallerSavedContext.r2;
	regsdump.basic.r3 = dfmTrapCallerSavedContext.r3;
	regsdump.basic.r12 = dfmTrapCallerSavedContext.r12;
	
	/* Populate the registers not included in the hardware exception frame. */
#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
	/*
	 * The hardware exception frame contains r0-r3, r12, lr, pc and xPSR.
	 * prvDfmTriggerCoredump() supplies the missing callee-saved state from the
	 * same SVC boundary, including the frame-pointer value needed for reliable
	 * unwinding.
	 */
	/* _callee_saved_t.psp must point to the hardware exception frame created
	 * by the SVC. This post-SVC address is deliberately assigned here rather
	 * than saving the pre-SVC PSP in the assembly shim. */
	dfmTrapCalleeSavedContext.psp = (uint32_t)(uintptr_t)exc_frame;
	regsdump.extra_info.callee =
		(_callee_saved_t *)&dfmTrapCalleeSavedContext;
	regsdump.extra_info.msp = dfmTrapMspBeforeSvc;
	regsdump.extra_info.exc_return = exc_return;
#endif	

	/* Sets the sp value correctly (so it excludes the hw-stacked exception frame) */
	z_arm_set_fault_sp((struct arch_esf*)exc_frame, exc_return);

	/* Now the regsdump object is ready to enable a regular Zephyr coredump() call. */
	coredump(K_ERR_DFM_TRAP, &regsdump, k_current_get());
}

void __attribute__((noinline)) prvDfmTrap(void)
{
	/* Check here, after DFM_TRAP has captured r0-r3/r12, but before using
	 * kernel services. Moving this check into the macro would clobber the
	 * caller-saved register snapshot for initialized traps. */
	if (ulDfmIsInitialized() == 0U)
	{
		return;
	}

	if (in_msp_context())
	{
		/* Generate an alert without coredump (not supported in MSP/handler mode) */
		prvDfmTrap_NoCoreDump(dfmTrapInfo.alertType, dfmTrapInfo.message,
			dfmTrapInfo.file, dfmTrapInfo.line, dfmTrapInfo.restart);
		return;
	}

	/*
	 * Use Zephyr's IRQ-offload SVC handler, but trigger it from the DFM
	 * assembly shim. This captures r4-r11 at exception entry and avoids an
	 * arch_irq_offload frame whose unwind rules depend on optimization level.
	 * The callback does not use Zephyr's private offload parameter.
	 */
	k_sched_lock();
	offload_routine = prvDfmRunCoreDump;
	prvDfmTriggerCoredump();
	offload_routine = (irq_offload_routine_t)0;
	k_sched_unlock();
}

#else /* DFM coredumps unavailable, IRQ offload unavailable, or not Cortex-M */

/* If DFM coredump is not supported (in general or in this case) */

void prvDfmTrap(int alertType, const char *message, const char *file, int line, int restart)
{
	prvDfmTrap_NoCoreDump(alertType, message, file, line, restart);
}

#endif

#endif

