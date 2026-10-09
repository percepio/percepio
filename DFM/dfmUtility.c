/*
 * Percepio DFM
 * Copyright 2026 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * DFM utility functions
 */

#include <string.h>
#include <dfm.h>

#if ((DFM_CFG_ENABLED) >= 1)

#if ((DFM_CFG_CRASH_ADD_TRACE) >= 1)
#include <trcRecorder.h>
#endif

const char * szDfmGetFileNameFromPath(const char *szPath)
{
	const size_t len = strlen(szPath);
	size_t i;
	
	if (len > 0)	/* since i is unsigned */
	{
		for (i = len; i > 0; i--)
		{
			char ch = szPath[i - 1];

			if (ch == '/' || ch == '\\')
			{
				szPath = &szPath[i];
				break;
			}
		}
	}

	return szPath;
}

uint32_t ulDfmCalculateChecksum(const char *ptr, size_t maxlen)
{
	uint32_t chksum = 0;
	size_t i;

	if (ptr != (void*)0)
	{
		for (i = 0; (ptr[i] != '\0') && (i < maxlen); i++)
		{
			chksum += (uint32_t) ptr[i];
		}
	}

	return chksum;
}

uint16_t usDfmCalculateCrc16Ccitt(uint16_t usSeed, const uint8_t* pucData, uint32_t ulSize)
{
	uint16_t usCrc = usSeed;
	uint32_t i;

	/*
	 * Keep this implementation local to DFM and software-only. Zephyr's
	 * hardware CRC implementation uses a semaphore, so it cannot be used
	 * safely from fault handlers where DFM may calculate this checksum.
	 */
	for (i = 0U; i < ulSize; i++)
	{
		uint16_t usE = (uint16_t)((usCrc ^ pucData[i]) & UINT16_C(0x00FF));
		uint16_t usF = (uint16_t)((usE ^ (uint16_t)(usE << 4)) & UINT16_C(0x00FF));

		usCrc = (uint16_t)((usCrc >> 8) ^ (uint16_t)(usF << 8) ^
			(uint16_t)(usF << 3) ^ (usF >> 4));
	}

	return usCrc;
}

DfmResult_t xDfmAddFileAndLineSymptoms(DfmAlertHandle_t xAlertHandle, const char *szFileName, int iLineNumber)
{
	DfmResult_t r1, r2;
	
#ifdef DFM_SYMPTOM_FILE
	if (szFileName != (void*)0)
	{
		r1 = xDfmAlertAddSymptom(xAlertHandle, DFM_SYMPTOM_FILE, ulDfmCalculateChecksum(szFileName, 32));
	}
	else
#endif
	{
		r1 = DFM_FAIL;
	}

#ifdef DFM_SYMPTOM_LINE
	if (iLineNumber != 0)
	{
		r2 = xDfmAlertAddSymptom(xAlertHandle, DFM_SYMPTOM_LINE, iLineNumber);
	}
	else
#endif
	{
		r2 = DFM_FAIL;
	}

	return r1 == DFM_SUCCESS && r2 == DFM_SUCCESS ? DFM_SUCCESS : DFM_FAIL;
}

#if ((DFM_CFG_CRASH_ADD_TRACE) >= 1)
DfmResult_t vDfmAddTracePayload(DfmAlertHandle_t xAlertHandle)
{
	void *pvBuffer = (void*)0;
	uint32_t ulBufferSize = 0;
	DfmResult_t r;
    
	/* Note that tracing must already be disabled at this point. */
	if (TRC_SUCCESS == xTraceGetEventBuffer(&pvBuffer, &ulBufferSize))
	{
		r = xDfmAlertAddPayload(xAlertHandle, pvBuffer, ulBufferSize, "dfm_trace.psfs");
	}
	else
	{
		r = DFM_FAIL;
	}
	return r;
}
#endif

#endif /* DFM_CFG_ENABLED */
