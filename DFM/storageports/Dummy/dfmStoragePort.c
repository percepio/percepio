/*
 * Percepio DFM v2.0.0
 * Copyright 2023 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Dummy Storage port
 */

#include <dfm.h>
#include <dfmStoragePort.h>

#if (defined(DFM_CFG_ENABLED) && ((DFM_CFG_ENABLED) >= 1))

DfmResult_t xDfmStoragePortInitialize(DfmStoragePortData_t *pxBuffer)
{
	(void)pxBuffer;

	return DFM_SUCCESS;
}

DfmResult_t xDfmStoragePortStoreSession(void* pvData, uint32_t ulSize)
{
	(void)pvData;
	(void)ulSize;

	return DFM_FAIL;
}

DfmResult_t xDfmStoragePortGetSession(void* pvBuffer, uint32_t ulBufferSize)
{
	(void)pvBuffer;
	(void)ulBufferSize;

	return DFM_FAIL;
}


DfmResult_t xDfmStoragePortStoreAlert(DfmEntryHandle_t xEntryHandle, uint32_t ulOverwrite)
{
	(void)xEntryHandle;
	(void)ulOverwrite;

	return DFM_FAIL;
}

DfmResult_t xDfmStoragePortGetAlert(void* pvBuffer, uint32_t ulBufferSize)
{
	(void)pvBuffer;
	(void)ulBufferSize;

	return DFM_FAIL;
}

DfmResult_t xDfmStoragePortStorePayloadChunk(DfmEntryHandle_t xEntryHandle, uint32_t ulOverwrite)
{
	(void)xEntryHandle;
	(void)ulOverwrite;

	return DFM_FAIL;
}

DfmResult_t xDfmStoragePortGetPayloadChunk(char* szSessionId, uint32_t ulAlertId, void* pvBuffer, uint32_t ulBufferSize)
{
	(void)szSessionId;
	(void)ulAlertId;
	(void)pvBuffer;
	(void)ulBufferSize;

	return DFM_FAIL;
}

#endif
