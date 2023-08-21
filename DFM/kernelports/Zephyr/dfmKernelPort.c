/*
 * Percepio DFM v2.0.0
 * Copyright 2023 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr Kernel port
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <dfm.h>

#if DFM_CFG_ENABLE_COREDUMPS == 1
#include <zephyr/debug/coredump.h>
#endif

/* Scratch related includes */
#include <zephyr/storage/flash_map.h>
#include <dfmConfig.h>

#if ((DFM_CFG_ENABLED) == 1)

#define MAX_COREDUMP_PARTS 4

DfmKernelPortData_t* pxKernelPortData;

DfmResult_t xDfmKernelPortInitialize(DfmKernelPortData_t *pxBuffer)
{
	if (pxBuffer == (void*)0)
	{
		return DFM_FAIL;
	}

	pxKernelPortData = pxBuffer;

	return DFM_SUCCESS;
}

//TODO: since we're just referring to a variable, we should possibily point to a const char
DfmResult_t xDfmKernelPortGetCurrentTaskName(char** pszTaskName)
{
	//TODO: Possbily add additional checking here
	k_tid_t current_thread = k_current_get();
	*pszTaskName = k_thread_name_get(current_thread);
	return DFM_SUCCESS;
}

/**
 * @brief Initialize aspects of the devalert module that must 
 * preceed the kernel initialization (scheduling, threads, etc.).
 * 
 * @param[in] arg
 */
static int devalert_pre_kernel_init(void)
{
	return 0;
}

/**
 * @brief Initialize aspects of the devalert module that depends on
 * the kernel being initialized.
 * 
 * @param[in] arg
 */
static int devalert_post_kernel_init(void)
{
	return 0;
}

/* Specify recorder module initialization stages */
SYS_INIT(devalert_pre_kernel_init, PRE_KERNEL_2, 0);
SYS_INIT(devalert_post_kernel_init, POST_KERNEL, 99);

/*
 * Functions for the coredump backend api, which in turn will use the dfmBackend for storing data
 * TODO: We probably don't want this in the kernel port but rather in its own file, like with crashcatcher.
 */

enum eDfmCoredumpState {
	DFM_COREDUMP_STATE_STARTED,
	DFM_COREDUMP_STATE_HEADER,
	DFM_COREDUMP_STATE_DATA
};

typedef struct DfmCoredumpPayload {
	uint8_t pubHeaderBuffer[32]; /* The header structs are packed, this is way more than the size they'll ever reach */
	size_t ulHeaderSize;
	void* pxContent;
	size_t ulContentSize;
} DfmCoredumpPayload_t;

#if DFM_CFG_ENABLE_COREDUMPS == 1

static enum eDfmCoredumpState eCoreDumpState = DFM_COREDUMP_STATE_STARTED;
static DfmCoredumpPayload_t pxDfmCoredumpParts[MAX_COREDUMP_PARTS]; // The zephyr coredump process won't add more than 3 headers
static uint8_t ubDfmCoreDumpHeaderCounter = 0;
static uint8_t ubDfmPayloadBuffer[CONFIG_PERCEPIO_DFM_CFG_MAX_COREDUMP_SIZE];
static uint32_t ulDfmPayloadBufferBytesUsed = 0;

DfmResult_t xDfmAlertAddCoredump(DfmAlertHandle_t xAlertHandle)
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
		if (ulContentSize > CONFIG_PERCEPIO_DFM_CFG_STACKDUMP_SIZE)
			ulContentSize = CONFIG_PERCEPIO_DFM_CFG_STACKDUMP_SIZE;

		/* Initial size check to avoid overflowing the buffer, i.e. does this part of the chunk fit within the chunk */
		if (ulDfmPayloadBufferBytesUsed + pxCurrentPayload->ulHeaderSize + ulContentSize > CONFIG_PERCEPIO_DFM_CFG_MAX_COREDUMP_SIZE)
			return DFM_FAIL;

		/* Copy the header */
		memcpy(pubDfmPayloadBufferPosition, pxCurrentPayload->pubHeaderBuffer, pxCurrentPayload->ulHeaderSize);
		pubDfmPayloadBufferPosition += pxCurrentPayload->ulHeaderSize;
		ulDfmPayloadBufferBytesUsed += pxCurrentPayload->ulHeaderSize;

		if (pxCurrentPayload->pxContent != NULL)
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
		"Coredump.dmp"
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
			/* TODO: Might be nice with some assertion here or similar */
			if (ulBufferLength > 32)
				return;

			pxCurrentPayload->ulHeaderSize = ulBufferLength;
			memcpy(pxCurrentPayload->pubHeaderBuffer, pxBuffer, ulBufferLength);
			ubDfmCoreDumpHeaderCounter++;
			eCoreDumpState = DFM_COREDUMP_STATE_HEADER;
			break;
		}

		case DFM_COREDUMP_STATE_HEADER:
		{
			if (ulBufferLength > 32)
				return;

			pxCurrentPayload->ulHeaderSize = ulBufferLength;
			memcpy(pxCurrentPayload->pubHeaderBuffer, pxBuffer, ulBufferLength);
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

/**
 * Finalize a coredump, which will result in, as long as the coredump reason is > 0xFFFF0000, the coredump parts recorded
 * by the xDfmCoredumpBackendBufferOutput are dumped to the disk and an alert is generated.
 * If the reason is > 0xFFFF0000, this is considered a user generated coredump, and thus, the user will need to call xDfmAlertAddCoredump
 * to append the coredump to the appropriate alert.
 * This function is called from the the Zephyr kernel.
 */
static void xDfmCoredumpBackendEnd(void)
{
	/* Examine the header to see whether this was a coredump created by the user or triggered from Zephyr */
	struct coredump_hdr_t* pxCoredumpHeader = (struct coredump_hdr_t*)(&pxDfmCoredumpParts[0]);

	/*
	 * There can exist architecture specific errors, so using a really high value here makes it possible for us to
	 * determine whether this coredump was triggered by a user or not.
	 * Since this is user triggered, the user will want to append these coredumps rather than having an alert created.
	 */
	if (pxCoredumpHeader->reason > 0xFFFF0000)
	{
		return;
	}

	DfmAlertHandle_t xAlertHandle;
	if (xDfmAlertBegin(DFM_TYPE_HARDFAULT, "HardfaultCoreD", &xAlertHandle) == DFM_SUCCESS)
	{
		/* TODO: Look into how to add various symptoms caught by the coredump here */
		xDfmAlertAddCoredump(xAlertHandle);

		/* Add the reason as a symptom */
		xDfmAlertAddSymptom(xAlertHandle, DFM_SYMPTOM_HARDFAULT_REASON, pxCoredumpHeader->reason);

#if defined(CONFIG_PERCEPIO_DFM_CFG_ADD_TRACE) && CONFIG_PERCEPIO_DFM_CFG_ADD_TRACE == 1
		xDfmAlertAddTrace(xAlertHandle);
#endif
		xDfmAlertEnd(xAlertHandle);
	}

}

static int xDfmCoredumpBackendCmd(enum coredump_cmd_id eCmdId, void *arg)
{
	return 0;
}

static int xDfmCoredumpBackendQuery(enum coredump_query_id eQueryId, void *arg)
{
	/* TODO: Might want to implement a few commands here, such as the size of the coredump */
	return 0;
}

struct coredump_backend_api coredump_backend_other = {
	.start = xDfmCoredumpBackendStart,
	.end = xDfmCoredumpBackendEnd,
	.buffer_output = xDfmCoredumpBackendBufferOutput,
	.cmd = xDfmCoredumpBackendCmd,
	.query = xDfmCoredumpBackendQuery
};

#endif

#if defined(CONFIG_PERCEPIO_TRACERECORDER) && CONFIG_PERCEPIO_TRACERECORDER == 1 && defined(CONFIG_PERCEPIO_TRC_CFG_STREAM_PORT_RINGBUFFER)
DfmResult_t xDfmAlertAddTrace(DfmAlertHandle_t xAlertHandle)
{
	void* pvBuffer = (void*)0;
	uint32_t ulBufferSize = 0;

	if(xTraceIsRecorderEnabled() == 1)
	{
		xTraceDisable();
	}
	else
	{
		return DFM_FAIL;
	}

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

#endif
