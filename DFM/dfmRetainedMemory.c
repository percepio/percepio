/*
 * Percepio DFM v2.1.0
 * Copyright 2023 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * DFM Retained Memory
 */

#include <dfm.h>

#if (defined(DFM_CFG_ENABLED) && (DFM_CFG_ENABLED >= 1)) && (defined(DFM_CFG_RETAINED_MEMORY) && (DFM_CFG_RETAINED_MEMORY >= 1))

#include <dfmRetainedMemoryPort.h>

#define DFM_RETAINED_MEMORY_TYPE_ALERT 0x341562AB
#define DFM_RETAINED_MEMORY_TYPE_PAYLOAD 0xE78BAC01

static DfmRetainedMemoryData_t* pxRetainedMemoryData;

typedef struct DfmRetainedMemoryMetaData
{
	uint32_t ulType;
	uint32_t ulSize;
} DfmRetainedMemoryMetaData_t;

static DfmResult_t prvRetainedMemoryWrite(uint32_t ulType, DfmEntryHandle_t xEntryHandle);
static DfmResult_t prvRetainedMemoryRead(uint32_t ulType, void* pvBuffer, uint32_t ulBufferSize);

static DfmResult_t prvRetainedMemoryWrite(uint32_t ulType, DfmEntryHandle_t xEntryHandle)
{
	DfmRetainedMemoryMetaData_t xRetainedMemoryMetaData;
	
	if (pxRetainedMemoryData == (void*) 0)
	{
		return DFM_FAIL;
	}
	
	if (pxRetainedMemoryData->ulInitialized == 0)
	{
		return DFM_FAIL;
	}
	
	if (xEntryHandle == 0)
	{
		return DFM_FAIL;
	}
	
	xRetainedMemoryMetaData.ulType = ulType;
	if (xDfmEntryGetSize(xEntryHandle, &xRetainedMemoryMetaData.ulSize) == DFM_FAIL)
	{
		return DFM_FAIL;
	}
	
	if (xRetainedMemoryMetaData.ulSize == 0)
	{
		return DFM_FAIL;
	}
	
	/* Write the meta data */
	if (xDfmRetainedMemoryPortWrite((uint8_t*)&xRetainedMemoryMetaData, sizeof(DfmRetainedMemoryMetaData_t), pxRetainedMemoryData->ulWriteOffset) == DFM_FAIL)
	{
		return DFM_FAIL;
	}
	
	pxRetainedMemoryData->ulWriteOffset += sizeof(DfmRetainedMemoryMetaData_t);

	/* Write the data */
	if (xDfmRetainedMemoryPortWrite((uint8_t*)xEntryHandle, xRetainedMemoryMetaData.ulSize, pxRetainedMemoryData->ulWriteOffset) == DFM_FAIL)
	{
		return DFM_FAIL;
	}
	
	pxRetainedMemoryData->ulWriteOffset += xRetainedMemoryMetaData.ulSize;
	
	return DFM_SUCCESS;
}

static DfmResult_t prvRetainedMemoryRead(uint32_t ulType, void* pvBuffer, uint32_t ulBufferSize)
{
	DfmRetainedMemoryMetaData_t xRetainedMemoryMetaData;
	
	/* Read the metadata */
	if (xDfmRetainedMemoryPortRead(&xRetainedMemoryMetaData, sizeof(DfmRetainedMemoryMetaData_t), pxRetainedMemoryData->ulReadOffset) == DFM_FAIL)
	{
		return DFM_FAIL;
	}
	
	if (xRetainedMemoryMetaData.ulType != ulType)
	{
		return DFM_FAIL;
	}
	
	if (xRetainedMemoryMetaData.ulSize <= 0)
	{
		return DFM_FAIL;
	}
	
	if (xRetainedMemoryMetaData.ulSize > ulBufferSize)
	{
		return DFM_FAIL;
	}
	
	pxRetainedMemoryData->ulReadOffset += sizeof(DfmRetainedMemoryMetaData_t);

	/* Read the data */
	if (xDfmRetainedMemoryPortRead(pvBuffer, xRetainedMemoryMetaData.ulSize, pxRetainedMemoryData->ulReadOffset) != 0)
	{
		return DFM_FAIL;
	}
	
	pxRetainedMemoryData->ulReadOffset += xRetainedMemoryMetaData.ulSize;
	
	return DFM_SUCCESS;
}

DfmResult_t xDfmRetainedMemoryInitialize(DfmRetainedMemoryData_t *pxBuffer)
{
	pxRetainedMemoryData = pxBuffer;

	pxRetainedMemoryData->ulWriteOffset = 0;
	pxRetainedMemoryData->ulReadOffset = 0;

	if (xDfmRetainedMemoryPortInitialize(&pxRetainedMemoryData->xRetainedMemoryPortData) == DFM_FAIL)
	{
		return DFM_FAIL;
	}

	pxRetainedMemoryData->ulInitialized = 1;

	return DFM_SUCCESS;
}

DfmResult_t xDfmRetainedMemoryWriteAlert(DfmEntryHandle_t xEntryHandle)
{
	pxRetainedMemoryData->ulWriteOffset = 0;
	pxRetainedMemoryData->ulReadOffset = 0;
	
	if (xDfmRetainedMemoryPortClear() == DFM_FAIL)
	{
		return DFM_FAIL;
	}

	return prvRetainedMemoryWrite(DFM_RETAINED_MEMORY_TYPE_ALERT, xEntryHandle);
}

DfmResult_t xDfmRetainedMemoryReadAlert(void* pvBuffer, uint32_t ulBufferSize)
{
	pxRetainedMemoryData->ulWriteOffset = 0;
	pxRetainedMemoryData->ulReadOffset = 0;

	/* We only check this on Read Alert since Read Payload will not be called if there is no Alert */
	if (xDfmRetainedMemoryPortHasData() == 0)
	{
		return DFM_FAIL;
	}

	return prvRetainedMemoryRead(DFM_RETAINED_MEMORY_TYPE_ALERT, pvBuffer, ulBufferSize);
}

DfmResult_t xDfmRetainedMemoryWritePayloadChunk(DfmEntryHandle_t xEntryHandle)
{
	return prvRetainedMemoryWrite(DFM_RETAINED_MEMORY_TYPE_PAYLOAD, xEntryHandle);
}

DfmResult_t xDfmRetainedMemoryReadPayloadChunk(char* szSessionId, uint32_t ulAlertId, void* pvBuffer, uint32_t ulBufferSize)
{
	(void)szSessionId;
	(void)ulAlertId;
	
	return prvRetainedMemoryRead(DFM_RETAINED_MEMORY_TYPE_PAYLOAD, pvBuffer, ulBufferSize);
}

DfmResult_t xDfmRetainedMemoryClear(void)
{
	return xDfmRetainedMemoryPortClear();
}

#endif
