/*
 * Percepio DFM
 * Copyright 2023-2026 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * DFM
 */

#include <dfm.h>

#if ((DFM_CFG_ENABLED) >= 1)

#if (!defined(DFM_VERSION) || (DFM_VERSION < DFM_VERSION_INITIAL) || (DFM_VERSION > DFM_VERSION_2_0))
#error "Invalid DFM_VERSION!"
#endif

#include <assert.h>
#include <string.h>
#include <stdint.h>

static DfmData_t xDfmData;
static DfmData_t* pxDfmData = &xDfmData;

DfmUserCallback_t xDfmUserGetUniqueSessionID;
DfmUserCallback_t xDfmUserGetDeviceName;

/* Shared buffer for snprintf calls, extern declared in dfm.h and used by dfmStopwatch, dfmTaskMonitor and dfmCrashCatcher. */
char cDfmPrintBuffer[128];

DfmResult_t prvDefaultGetDeviceName(char cBuffer[], uint32_t ulSize, uint32_t* pulBytesWritten)
{
	uint32_t nBytes;
	nBytes = snprintf(cBuffer, ulSize, "$DUMMY_DEVICE_ID"); // This makes percepio-receiver.py set a provided DeviceID instead (i.e. on host).

	if (nBytes > 0)
	{
		*pulBytesWritten = nBytes;
		return DFM_SUCCESS;
	}

	return DFM_FAIL;
}



DfmResult_t prvDefaultSessionID(char cBuffer[], uint32_t ulSize, uint32_t* pulBytesWritten)
{
	uint32_t nBytes;
	nBytes = snprintf(cBuffer, ulSize, "$DUMMY_SESSION_ID"); // This makes percepio-receiver.py set the current host time as SessionID instead (i.e. on host).

	if (nBytes > 0)
	{
		*pulBytesWritten = nBytes;
		return DFM_SUCCESS;
	}

	return DFM_FAIL;
}

DfmResult_t xDfmInitializeForLocalUse(void)
{
	return xDfmInitialize(prvDefaultSessionID, prvDefaultGetDeviceName);
}


DfmResult_t xDfmInitialize(DfmUserCallback_t xGetUniqueSessionID, DfmUserCallback_t xGetDeviceName)
{
	if (pxDfmData == (void*)0)
	{
		DFM_ERROR_PRINT("xDfmInitialize Error - pxDfmData is NULL\n");
		return DFM_FAIL;
	}

	if (xGetUniqueSessionID == 0)
	{
		DFM_ERROR_PRINT("xDfmInitialize Error - xGetUniqueSessionID is 0\n");
		return DFM_FAIL;
	}

	if (xGetDeviceName == 0)
	{
		DFM_ERROR_PRINT("xDfmInitialize Error - xGetDeviceName is 0\n");
		return DFM_FAIL;
	}

	(void)memset(pxDfmData, 0, sizeof(DfmData_t));

	xDfmUserGetUniqueSessionID = xGetUniqueSessionID;
	xDfmUserGetDeviceName = xGetDeviceName;

	if (xDfmSessionInitialize(&pxDfmData->xSessionData) == DFM_FAIL)
	{
		DFM_ERROR_PRINT("xDfmInitialize Error - xDfmSessionInitialize failed.\n");
		return DFM_FAIL;
	}

	if (xDfmKernelPortInitialize(&pxDfmData->xKernelPortData) == DFM_FAIL)
	{
		DFM_ERROR_PRINT("xDfmInitialize Error - xDfmKernelPortInitialize failed.\n");
		return DFM_FAIL;
	}

	if (xDfmAlertInitialize(&pxDfmData->xAlertData) == DFM_FAIL)
	{
		DFM_ERROR_PRINT("xDfmInitialize Error - xDfmAlertInitialize failed.\n");
		return DFM_FAIL;
	}

	if (xDfmEntryInitialize(&pxDfmData->xEntryData) == DFM_FAIL)
	{
		DFM_ERROR_PRINT("xDfmInitialize Error - xDfmEntryInitialize failed.\n");
		return DFM_FAIL;
	}

	if (xDfmCloudInitialize(&pxDfmData->xCloudData) == DFM_FAIL)
	{
		DFM_ERROR_PRINT("xDfmInitialize Error - xDfmCloudInitialize failed.\n");
		return DFM_FAIL;
	}

	if (xDfmStorageInitialize(&pxDfmData->xStorageData) == DFM_FAIL)
	{
		DFM_ERROR_PRINT("xDfmInitialize Error - xDfmStorageInitialize failed.\n");
		return DFM_FAIL;
	}

#if (defined(DFM_CFG_RETAINED_MEMORY) && (DFM_CFG_RETAINED_MEMORY >= 1))
	if (xDfmRetainedMemoryInitialize(&pxDfmData->xRetainedMemoryData) == DFM_FAIL)
	{
		return DFM_FAIL;
	}
#endif

	(void)xDfmTaskMonitorInit();

	pxDfmData->ulDfmInitialized = 1;

	(void)xDfmSessionEnable(0); /* Try to enable, but don't override if disabled */

	return DFM_SUCCESS;
}

uint32_t ulDfmIsInitialized(void)
{
	return xDfmData.ulDfmInitialized;
}

#endif
