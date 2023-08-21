/*
 * Percepio DFM v2.0.0
 * Copyright 2023 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * DFM Storage
 */

#include <dfm.h>
#include <string.h>

#if ((DFM_CFG_ENABLED) >= 1)

static DfmStorageData_t* pxStorageData = (void*)0;

#define DFM_STORAGE_VERSION 1
#define DFM_STORAGE_ALERT_TYPE 0x1512
#define DFM_STORAGE_PAYLOAD_TYPE 0x8371

DfmResult_t xDfmStorageInitialize(DfmStorageData_t* pxBuffer)
{
	if (pxBuffer == (void*)0)
	{
		return DFM_FAIL;
	}

	pxStorageData = pxBuffer;

	if (xDfmStoragePortInitialize(&pxStorageData->xStoragePortData) == DFM_FAIL) /*cstat !MISRAC2012-Rule-14.3_b The mocked storage port used for MISRA always returns DFM_SUCCESS so this check will never be true. In a real system that will not be the case.*/
	{
		return DFM_FAIL;
	}

	pxStorageData->ulInitialized = 1;

	return DFM_SUCCESS;
}

DfmResult_t xDfmStorageStoreSession(void* pvSession, uint32_t ulSessionSize)
{
	if (pxStorageData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxStorageData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (ulDfmSessionIsEnabled() == 0UL)
	{
		return DFM_FAIL;
	}

	if (pvSession == (void*)0)
	{
		return DFM_FAIL;
	}

	if (ulSessionSize == 0UL)
	{
		return DFM_FAIL;
	}

	return xDfmStoragePortStoreSession(pvSession, ulSessionSize);
}

DfmResult_t xDfmStorageGetSession(void* pvBuffer, uint32_t ulBufferSize)
{
	if (pxStorageData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxStorageData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (ulDfmSessionIsEnabled() == 0UL)
	{
		return DFM_FAIL;
	}

	if (pvBuffer == (void*)0)
	{
		return DFM_FAIL;
	}

	if (ulBufferSize == 0UL)
	{
		return DFM_FAIL;
	}

	return xDfmStoragePortGetSession(pvBuffer, ulBufferSize);
}

DfmResult_t xDfmStorageStoreAlert(DfmEntryHandle_t xEntryHandle)
{
	DfmStorageStrategy_t xStorageStrategy = DFM_STORAGE_STRATEGY_SKIP;

	if (pxStorageData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxStorageData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (ulDfmSessionIsEnabled() == 0UL)
	{
		return DFM_FAIL;
	}

	if (xEntryHandle == 0)
	{
		return DFM_FAIL;
	}

	if (xDfmSessionGetStorageStrategy(&xStorageStrategy) == DFM_FAIL)
	{
		return DFM_FAIL;
	}

	if (xStorageStrategy == DFM_STORAGE_STRATEGY_IGNORE)
	{
		return DFM_FAIL;
	}
	
	if (xDfmStoragePortStoreAlert(xEntryHandle, (uint32_t)(xStorageStrategy == DFM_STORAGE_STRATEGY_OVERWRITE)) == DFM_FAIL) /*cstat !MISRAC2012-Rule-14.3_b The mocked storage port used for MISRA always returns DFM_SUCCESS so this check will never be true. In a real system that will not be the case.*/
	{
		return DFM_FAIL;
	}

	return DFM_SUCCESS;
}

DfmResult_t xDfmStorageGetAlert(void* pvBuffer, uint32_t ulBufferSize)
{
	if (pxStorageData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxStorageData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (ulDfmSessionIsEnabled() == 0UL)
	{
		return DFM_FAIL;
	}

	if (pvBuffer == (void*)0)
	{
		return DFM_FAIL;
	}

	if (ulBufferSize == 0UL)
	{
		return DFM_FAIL;
	}

	if (xDfmStoragePortGetAlert(pvBuffer, ulBufferSize) == DFM_FAIL) /*cstat !MISRAC2012-Rule-14.3_b The mocked storage port used for MISRA always returns DFM_SUCCESS so this check will never be true. In a real system that will not be the case.*/
	{
		return DFM_FAIL;
	}

	return DFM_SUCCESS;
}

DfmResult_t xDfmStorageStorePayloadChunk(DfmEntryHandle_t xEntryHandle)
{
	DfmStorageStrategy_t xStorageStrategy = DFM_STORAGE_STRATEGY_SKIP;

	if (pxStorageData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxStorageData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (ulDfmSessionIsEnabled() == 0UL)
	{
		return DFM_FAIL;
	}

	if (xEntryHandle == 0)
	{
		return DFM_FAIL;
	}

	if (xDfmSessionGetStorageStrategy(&xStorageStrategy) == DFM_FAIL)
	{
		return DFM_FAIL;
	}

	if (xStorageStrategy == DFM_STORAGE_STRATEGY_IGNORE)
	{
		return DFM_FAIL;
	}

	if (xDfmStoragePortStorePayloadChunk(xEntryHandle, (uint32_t)(xStorageStrategy == DFM_STORAGE_STRATEGY_OVERWRITE)) == DFM_FAIL) /*cstat !MISRAC2012-Rule-14.3_b The mocked storage port used for MISRA always returns DFM_SUCCESS so this check will never be true. In a real system that will not be the case.*/
	{
		return DFM_FAIL;
	}

	return DFM_SUCCESS;
}

DfmResult_t xDfmStorageGetPayloadChunk(char* szSessionId, uint32_t ulAlertId, void* pvBuffer, uint32_t ulBufferSize)
{
	if (pxStorageData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxStorageData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (ulDfmSessionIsEnabled() == 0UL)
	{
		return DFM_FAIL;
	}

	if (szSessionId == (void*)0)
	{
		return DFM_FAIL;
	}

	if (szSessionId[0] == (char)0)
	{
		return DFM_FAIL;
	}

	if (pvBuffer == (void*)0)
	{
		return DFM_FAIL;
	}

	if (ulBufferSize == 0UL)
	{
		return DFM_FAIL;
	}

	/* TODO: 64-bit compatible */
	/* Make sure szSessiondId is not pointing to a string inside the buffer since that means this function is incorrectly used and it will be overwritten! */
	if (((uint32_t)szSessionId >= (uint32_t)pvBuffer) && ((uint32_t)szSessionId < (((uint32_t)pvBuffer) + ulBufferSize))) /*cstat !MISRAC2012-Rule-11.6 !MISRAC2012-Rule-18.3 !MISRAC2012-Rule-18.4 We need to verify that szSessionId is not pointing at an address inside pvBuffer. This means that we need to offset the buffer pointer by the buffer size before comparison.*/
	{
		return DFM_FAIL;
	}

	if (xDfmStoragePortGetPayloadChunk(szSessionId, ulAlertId, pvBuffer, ulBufferSize) == DFM_FAIL) /*cstat !MISRAC2012-Rule-14.3_b The mocked storage port used for MISRA always returns DFM_SUCCESS here so this check will never be true. In a real system that will not be the case.*/
	{
		return DFM_FAIL;
	}

	return DFM_SUCCESS;
}

#endif
