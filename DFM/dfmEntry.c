/*
 * Percepio DFM v2.0.0
 * Copyright 2023 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * DFM Entry
 */

#include <dfm.h>
#include <string.h>

#if ((DFM_CFG_ENABLED) >= 1)

#define DFM_ENTRY_VERSION 1

typedef struct DfmEntryHeader
{
	uint8_t cStartMarkers[4];
	uint16_t usEndianess;
	uint16_t usVersion;
	uint16_t usType;
	uint16_t usEntryId;
	uint16_t usChunkIndex;
	uint16_t usChunkCount;
	uint32_t ulAlertId;
	uint16_t usSessionIdSize;
	uint16_t usDeviceNameSize;
	uint16_t usDescriptionSize;
	uint16_t usReserved;
	uint32_t ulDataSize;
} DfmEntryHeader_t;

typedef struct DfmEntryPayloadHeader
{
	uint8_t ucStartMarkers[4];		/* The DFM Payload start markers, must be 0x50, 0x44, 0x61, 0x50 ("PDaP") */
	uint16_t usEndianness;			/* The endianness checker, assign to 0x0FF0 */
	uint8_t ucVersion;				/* The version of the DFM subsystem, current version is 3 */
	uint8_t ucFilenameSize;			/* The maximum length of cFilenameBuffer */
	uint32_t ulFileSize;			/* The size of the file (buffer) */
	char cFilenameBuffer[DFM_PAYLOAD_DESCRIPTION_MAX_LEN]; /* Size will always be 4-byte aligned */
	uint8_t ucEndMarkers[4];		/* The DFM Payload end markers, must be 0x50, 0x61, 0x44, 0x50 ("PaDP") */
	uint32_t ulChecksum;			/* Checksum on the whole thing, 0 for not enabled */
} DfmEntryPayloadHeader_t;

typedef struct DfmEntryFooter
{
	uint8_t cEndMarkers[4];
} DfmEntryFooter_t;

static DfmEntryData_t* pxDfmEntryData = (void*)0;

static DfmResult_t prvDfmEntryVerify(DfmEntryHandle_t xEntryHandle);
static uint32_t prvDfmEntryGetHeaderSize(void);
static DfmResult_t prvDfmEntrySetup(uint16_t usType, uint16_t usEntryId, uint16_t usChunkIndex, uint16_t usChunkCount, uint32_t ulDescriptionSize, const char* szDescription, void* pvData, uint32_t ulDataSize, DfmEntryHandle_t* pxEntryHandle);

/* This function is only used to get around constant "if" condition */
static uint32_t prvDfmEntryGetHeaderSize(void)
{
	return sizeof(DfmEntryHeader_t);
}

static DfmResult_t prvDfmEntryVerify(DfmEntryHandle_t xEntryHandle)
{
	uint32_t ulEntrySize;
	DfmEntryHeader_t* pxEntryHeader;
	uint8_t* cEndMarkers;

	pxEntryHeader = (DfmEntryHeader_t*)xEntryHandle;

	if (xDfmEntryGetSize(xEntryHandle, &ulEntrySize) == DFM_FAIL)
	{
		return DFM_FAIL;
	}

	if (ulEntrySize > sizeof(pxDfmEntryData->buffer))
	{
		return DFM_FAIL;
	}

	if ((pxEntryHeader->cStartMarkers[0] != (uint8_t)0xD1) ||
		(pxEntryHeader->cStartMarkers[1] != (uint8_t)0xD2) ||
		(pxEntryHeader->cStartMarkers[2] != (uint8_t)0xD3) ||
		(pxEntryHeader->cStartMarkers[3] != (uint8_t)0xD4))
	{
		return DFM_FAIL;
	}

	if (xDfmEntryGetEndMarkers(xEntryHandle, &cEndMarkers) == DFM_FAIL)
	{
		return DFM_FAIL;
	}

	if ((cEndMarkers[0] != (uint8_t)0xD4) ||
		(cEndMarkers[1] != (uint8_t)0xD3) ||
		(cEndMarkers[2] != (uint8_t)0xD2) ||
		(cEndMarkers[3] != (uint8_t)0xD1))
	{
		return DFM_FAIL;
	}

	if ((pxEntryHeader->usType != (uint16_t)DFM_ENTRY_TYPE_ALERT) &&
		(pxEntryHeader->usType != (uint16_t)DFM_ENTRY_TYPE_PAYLOAD) &&
		(pxEntryHeader->usType != (uint16_t)DFM_ENTRY_TYPE_PAYLOAD_HEADER))
	{
		return DFM_FAIL;
	}

	if (pxEntryHeader->usVersion > (uint16_t)DFM_ENTRY_VERSION)
	{
		return DFM_FAIL;
	}

	if (pxEntryHeader->usChunkIndex == (uint16_t)0)
	{
		return DFM_FAIL;
	}
	
	if (pxEntryHeader->usChunkIndex > pxEntryHeader->usChunkCount)
	{
		return DFM_FAIL;
	}

	return DFM_SUCCESS;
}

static DfmResult_t prvDfmEntrySetup(uint16_t usType, uint16_t usEntryId, uint16_t usChunkIndex, uint16_t usChunkCount, uint32_t ulDescriptionSize, const char* szDescription, void* pvData, uint32_t ulDataSize, DfmEntryHandle_t* pxEntryHandle)
{
	DfmEntryHeader_t* pxEntryHeader = (DfmEntryHeader_t*)pxDfmEntryData->buffer; /*cstat !MISRAC2012-Rule-11.3 We convert the untyped buffer to something we can work with. We can't use an exact type since the buffer may need to contain bigger Entries that haven been previously stored*/
	char* szSessionId = (void*)0;
	uint32_t ulOffset = 0UL;
	uint32_t ulAlertId = 0UL;
	const char* szDeviceName = (void*)0;

	if (szDescription == (void*)0)
	{
		return DFM_FAIL;
	}

	if (szDescription[0] == (char)0)
	{
		return DFM_FAIL;
	}

	if (pvData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (ulDataSize == 0UL)
	{
		return DFM_FAIL;
	}

	if (pxEntryHandle == (void*)0)
	{
		return DFM_FAIL;
	}

	if (usChunkIndex == (uint16_t)0)
	{
		return DFM_FAIL;
	}

	if (usChunkIndex > usChunkCount)
	{
		return DFM_FAIL;
	}

	if (xDfmSessionGetUniqueSessionId(&szSessionId) == DFM_FAIL)
	{
		return DFM_FAIL;
	}

	if (xDfmSessionGetAlertId(&ulAlertId) == DFM_FAIL)
	{
		return DFM_FAIL;
	}

	if (xDfmSessionGetDeviceName(&szDeviceName) == DFM_FAIL)
	{
		return DFM_FAIL;
	}

	pxEntryHeader->cStartMarkers[0] = 0xD1;
	pxEntryHeader->cStartMarkers[1] = 0xD2;
	pxEntryHeader->cStartMarkers[2] = 0xD3;
	pxEntryHeader->cStartMarkers[3] = 0xD4;

	pxEntryHeader->usEndianess = 0x0FF0;
	pxEntryHeader->usVersion = DFM_ENTRY_VERSION;
	pxEntryHeader->usType = usType;
	pxEntryHeader->usEntryId = usEntryId;
	pxEntryHeader->usChunkIndex = usChunkIndex;
	pxEntryHeader->usChunkCount = usChunkCount;
	pxEntryHeader->ulAlertId = ulAlertId;
	pxEntryHeader->usSessionIdSize = DFM_SESSION_ID_MAX_LEN;
	pxEntryHeader->usDeviceNameSize = DFM_DEVICE_NAME_MAX_LEN;
	pxEntryHeader->usDescriptionSize = (uint16_t)ulDescriptionSize;
	pxEntryHeader->ulDataSize = ulDataSize;

	ulOffset += sizeof(DfmEntryHeader_t);

	/* TODO: 64-bit support */
	/* Write SessionId after Header */
	(void)memcpy((char*)((uint32_t)pxDfmEntryData->buffer + ulOffset), szSessionId, DFM_SESSION_ID_MAX_LEN); /*cstat !MISRAC2012-Rule-11.6 We need to write the Session Id to the buffer with an offset*/

	ulOffset += (uint32_t)(DFM_SESSION_ID_MAX_LEN);
	
	/* TODO: 64-bit support */
	/* Write DeviceName after SessionId */
	(void)memcpy((char*)((uint32_t)pxDfmEntryData->buffer + ulOffset), szDeviceName, DFM_DEVICE_NAME_MAX_LEN); /*cstat !MISRAC2012-Rule-11.6 We need to write the Device Name to the buffer with an offset*/

	ulOffset += (uint32_t)(DFM_DEVICE_NAME_MAX_LEN);

	/* TODO: 64-bit support */
	/* Write Description after DeviceName */
	(void)memcpy((char*)((uint32_t)pxDfmEntryData->buffer + ulOffset), szDescription, ulDescriptionSize); /*cstat !MISRAC2012-Rule-11.6 We need to write the Description to the buffer with an offset*/

	ulOffset += ulDescriptionSize;

	/* TODO: 64-bit support */
	/* Write Data after Description */
	(void)memcpy((void*)((uint32_t)pxDfmEntryData->buffer + ulOffset), pvData, ulDataSize); /*cstat !MISRAC2012-Rule-11.6 We need to write the Entry data to the buffer with an offset*/

	ulOffset += ulDataSize;

	/* TODO: 64-bit support */
	/* Write Footer after Data */
	DfmEntryFooter_t* pxEntryFooter = (DfmEntryFooter_t*)((uint32_t)pxDfmEntryData->buffer + ulOffset);
	pxEntryFooter->cEndMarkers[0] = pxEntryHeader->cStartMarkers[3];
	pxEntryFooter->cEndMarkers[1] = pxEntryHeader->cStartMarkers[2];
	pxEntryFooter->cEndMarkers[2] = pxEntryHeader->cStartMarkers[1];
	pxEntryFooter->cEndMarkers[3] = pxEntryHeader->cStartMarkers[0];

	*pxEntryHandle = (DfmEntryHandle_t)pxEntryHeader;

	return DFM_SUCCESS;
}

/*
 * Since the sizes are dynamic, an Entry will be stored and read by interpreting consecutive parts as:
 * Alert: DfmEntryHeader_t -> session[ulSessionIdSize] -> description[ulDescriptionSize] -> alert[ulDataSize] -> DfmEntryFooter_t
 * Payload: DfmEntryHeader_t -> session[ulSessionIdSize] -> description[ulDescriptionSize] -> payload[ulDataSize] -> DfmEntryFooter_t
 */
DfmResult_t xDfmEntryInitialize(DfmEntryData_t* pxBuffer)
{
	if (pxBuffer == (void*)0)
	{
		return DFM_FAIL;
	}

	pxDfmEntryData = pxBuffer;

	/* Verify that entry buffer can hold alert size. Calls prvDfmEntryGetHeaderSize() to avoid constant "if" condition. */
	if ((prvDfmEntryGetHeaderSize() + (uint32_t)(DFM_SESSION_ID_MAX_LEN) + (uint32_t)(DFM_DESCRIPTION_MAX_LEN) + (uint32_t)(DFM_DEVICE_NAME_MAX_LEN) + sizeof(DfmAlert_t) + sizeof(DfmEntryFooter_t)) > sizeof(pxDfmEntryData->buffer))
	{
		return DFM_FAIL;
	}

	/* Verify that entry buffer can hold payload size. Calls prvDfmEntryGetHeaderSize() to avoid constant "if" condition. */
	if ((prvDfmEntryGetHeaderSize() + (uint32_t)(DFM_SESSION_ID_MAX_LEN) + (uint32_t)(DFM_PAYLOAD_DESCRIPTION_MAX_LEN) + (uint32_t)(DFM_DEVICE_NAME_MAX_LEN) + (uint32_t)(DFM_CFG_MAX_PAYLOAD_CHUNK_SIZE) + sizeof(DfmEntryFooter_t)) > sizeof(pxDfmEntryData->buffer))
	{
		return DFM_FAIL;
	}

	pxDfmEntryData->ulInitialized = 1;

	return DFM_SUCCESS;
}

DfmResult_t xDfmEntryGetBuffer(void** ppvBuffer, uint32_t* pulBufferSize)
{
	if (pxDfmEntryData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmEntryData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (ppvBuffer == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pulBufferSize == (void*)0)
	{
		return DFM_FAIL;
	}

	*ppvBuffer = pxDfmEntryData->buffer;
	*pulBufferSize = sizeof(pxDfmEntryData->buffer);

	return DFM_SUCCESS;
}

DfmResult_t xDfmEntryCreateAlert(DfmAlertHandle_t xAlertHandle, DfmEntryHandle_t *pxEntryHandle)
{
	const char* szDescription = (void*)0;

	if (pxDfmEntryData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmEntryData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (xAlertHandle == 0)
	{
		return DFM_FAIL;
	}

	if (xDfmAlertGetDescription(xAlertHandle, &szDescription) == DFM_FAIL)
	{
		return DFM_FAIL;
	}

	if (prvDfmEntrySetup((uint16_t)(DFM_ENTRY_TYPE_ALERT), (uint16_t)0, (uint16_t)1, (uint16_t)1, (uint32_t)(DFM_DESCRIPTION_MAX_LEN), szDescription, (void*)xAlertHandle, sizeof(DfmAlert_t), pxEntryHandle) == DFM_FAIL)
	{
		return DFM_FAIL;
	}

	return DFM_SUCCESS;
}

DfmResult_t xDfmEntryCreatePayloadHeader(DfmAlertHandle_t xAlertHandle, uint16_t usEntryId, uint32_t ulPayloadSize, char* szDescription, DfmEntryHandle_t* pxEntryHandle)
{
	DfmEntryPayloadHeader_t xPayloadHeader = { 0 };
	uint32_t i;

	if (pxDfmEntryData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmEntryData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (xAlertHandle == 0)
	{
		return DFM_FAIL;
	}

	if (ulPayloadSize == 0UL)
	{
		return DFM_FAIL;
	}

	if (szDescription == (void*)0)
	{
		return DFM_FAIL;
	}

	if (szDescription[0] == (char)0)
	{
		return DFM_FAIL;
	}

	if (pxEntryHandle == (void*)0)
	{
		return DFM_FAIL;
	}

	xPayloadHeader.ucStartMarkers[0] = 0x50;
	xPayloadHeader.ucStartMarkers[1] = 0x44;
	xPayloadHeader.ucStartMarkers[2] = 0x61;
	xPayloadHeader.ucStartMarkers[3] = 0x50;
	xPayloadHeader.usEndianness = 0x0FF0;
	xPayloadHeader.ucVersion = DFM_VERSION;
	xPayloadHeader.ucFilenameSize = (uint8_t)sizeof(xPayloadHeader.cFilenameBuffer);
	xPayloadHeader.ulFileSize = ulPayloadSize;

	for (i = 0; i < sizeof(xPayloadHeader.cFilenameBuffer); i++)
	{
		xPayloadHeader.cFilenameBuffer[i] = szDescription[i];

		if (xPayloadHeader.cFilenameBuffer[i] == (char)0)
		{
			break;
		}
	}

	xPayloadHeader.ucEndMarkers[0] = 0x50;
	xPayloadHeader.ucEndMarkers[1] = 0x61;
	xPayloadHeader.ucEndMarkers[2] = 0x44;
	xPayloadHeader.ucEndMarkers[3] = 0x50;
	xPayloadHeader.ulChecksum = 0;

	if (prvDfmEntrySetup((uint16_t)(DFM_ENTRY_TYPE_PAYLOAD_HEADER), usEntryId, (uint16_t)1, (uint16_t)1, (uint32_t)(DFM_PAYLOAD_DESCRIPTION_MAX_LEN), szDescription, &xPayloadHeader, sizeof(xPayloadHeader), pxEntryHandle) == DFM_FAIL)
	{
		return DFM_FAIL;
	}

	return DFM_SUCCESS;
}

DfmResult_t xDfmEntryCreatePayloadChunk(DfmAlertHandle_t xAlertHandle, uint16_t usEntryId, uint16_t usChunkIndex, uint16_t usChunkCount, void* pvPayload, uint32_t ulSize, char* szDescription, DfmEntryHandle_t* pxEntryHandle)
{
	if (pxDfmEntryData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmEntryData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (xAlertHandle == 0)
	{
		return DFM_FAIL;
	}
	
	if (prvDfmEntrySetup((uint16_t)(DFM_ENTRY_TYPE_PAYLOAD), usEntryId, usChunkIndex, usChunkCount, (uint32_t)(DFM_PAYLOAD_DESCRIPTION_MAX_LEN), szDescription, pvPayload, ulSize, pxEntryHandle) == DFM_FAIL)
	{
		return DFM_FAIL;
	}

	return DFM_SUCCESS;
}

DfmResult_t xDfmEntryCreateAlertFromBuffer(DfmEntryHandle_t* pxEntryHandle)
{
	DfmEntryHeader_t* pxEntryHeader;

	if (pxDfmEntryData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmEntryData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (pxEntryHandle == (void*)0)
	{
		return DFM_FAIL;
	}

	/* We assume the data is in the buffer */

	pxEntryHeader = (DfmEntryHeader_t*)pxDfmEntryData->buffer; /*cstat !MISRAC2012-Rule-11.3 We convert the untyped buffer to something we can work with. We can't use an exact type since the buffer may need to contain bigger Entries that haven been previously stored*/

	if (prvDfmEntryVerify((DfmEntryHandle_t)pxEntryHeader) == DFM_FAIL)
	{
		return DFM_FAIL;
	}

	if (pxEntryHeader->usType != (uint16_t)(DFM_ENTRY_TYPE_ALERT))
	{
		return DFM_FAIL;
	}

	*pxEntryHandle = (DfmEntryHandle_t)pxEntryHeader;

	return DFM_SUCCESS;
}

DfmResult_t xDfmEntryCreatePayloadChunkFromBuffer(const char* szSessionId, uint32_t ulAlertId, DfmEntryHandle_t* pxEntryHandle)
{
	DfmEntryHeader_t* pxEntryHeader;
	const char* szPayloadSession = (void*)0;
	uint32_t i;

	if (pxDfmEntryData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmEntryData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (pxEntryHandle == (void*)0)
	{
		return DFM_FAIL;
	}

	/* TODO: 64-bit compatible */
	/* Make sure szSessiondId is not pointing to a string inside the buffer since that means this function is incorrectly used! */
	if (((uint32_t)szSessionId >= (uint32_t)pxDfmEntryData->buffer) && ((uint32_t)szSessionId < (uint32_t)pxDfmEntryData->buffer + sizeof(pxDfmEntryData->buffer)))
	{
		return DFM_FAIL;
	}

	/* We assume the data is in the buffer */

	pxEntryHeader = (DfmEntryHeader_t*)pxDfmEntryData->buffer; /*cstat !MISRAC2012-Rule-11.3 We convert the untyped buffer to something we can work with. We can't use an exact type since the buffer may need to contain bigger Entries that haven been previously stored*/

	if (prvDfmEntryVerify((DfmEntryHandle_t)pxEntryHeader) == DFM_FAIL)
	{
		return DFM_FAIL;
	}

	if (pxEntryHeader->usType != (uint16_t)(DFM_ENTRY_TYPE_PAYLOAD) && pxEntryHeader->usType != (uint16_t)(DFM_ENTRY_TYPE_PAYLOAD_HEADER))
	{
		return DFM_FAIL;
	}

	/* Make sure stored alert id matches what we asked for */
	if (pxEntryHeader->ulAlertId != ulAlertId)
	{
		return DFM_FAIL;
	}

	if (xDfmEntryGetSessionId((DfmEntryHandle_t)pxEntryHeader, &szPayloadSession) == DFM_FAIL)
	{
		return DFM_FAIL;
	}

	/* Make sure stored session matches what we asked for */
	for (i = 0; i < (uint32_t)(DFM_SESSION_ID_MAX_LEN); i++)
	{
		if (szSessionId[i] != szPayloadSession[i])
		{
			return DFM_FAIL;
		}

		if (szSessionId[i] == (char)0)
		{
			break;
		}
	}

	*pxEntryHandle = (DfmEntryHandle_t)pxEntryHeader;

	return DFM_SUCCESS;
}

DfmResult_t xDfmEntryGetSize(DfmEntryHandle_t xEntryHandle, uint32_t *pulSize)
{
	DfmEntryHeader_t* pxEntryHeader;

	if (pxDfmEntryData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmEntryData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (xEntryHandle == 0)
	{
		return DFM_FAIL;
	}

	if (pulSize == (void*)0)
	{
		return DFM_FAIL;
	}

	pxEntryHeader = (DfmEntryHeader_t*)xEntryHandle;

	*pulSize = sizeof(DfmEntryHeader_t) + pxEntryHeader->usSessionIdSize + pxEntryHeader->usDeviceNameSize + pxEntryHeader->usDescriptionSize + pxEntryHeader->ulDataSize + sizeof(DfmEntryFooter_t);

	return DFM_SUCCESS;
}

DfmResult_t xDfmEntryGetStartMarkers(DfmEntryHandle_t xEntryHandle, uint8_t** pucMarkersBuffer)
{
	DfmEntryHeader_t* pxEntryHeader;

	if (pxDfmEntryData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmEntryData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (xEntryHandle == 0)
	{
		return DFM_FAIL;
	}

	if (pucMarkersBuffer == (void*)0)
	{
		return DFM_FAIL;
	}

	pxEntryHeader = (DfmEntryHeader_t*)xEntryHandle;

	*pucMarkersBuffer = pxEntryHeader->cStartMarkers;

	return DFM_SUCCESS;
}

DfmResult_t xDfmEntryGetEndianess(DfmEntryHandle_t xEntryHandle, uint16_t* pusEndianess)
{
	DfmEntryHeader_t* pxEntryHeader;

	if (pxDfmEntryData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmEntryData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (xEntryHandle == 0)
	{
		return DFM_FAIL;
	}

	if (pusEndianess == (void*)0)
	{
		return DFM_FAIL;
	}

	pxEntryHeader = (DfmEntryHeader_t*)xEntryHandle;

	*pusEndianess = pxEntryHeader->usEndianess;

	return DFM_SUCCESS;
}

DfmResult_t xDfmEntryGetVersion(DfmEntryHandle_t xEntryHandle, uint16_t* pusVersion)
{
	DfmEntryHeader_t* pxEntryHeader;

	if (pxDfmEntryData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmEntryData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (xEntryHandle == 0)
	{
		return DFM_FAIL;
	}

	if (pusVersion == (void*)0)
	{
		return DFM_FAIL;
	}

	pxEntryHeader = (DfmEntryHeader_t*)xEntryHandle;

	*pusVersion = pxEntryHeader->usVersion;

	return DFM_SUCCESS;
}

DfmResult_t xDfmEntryGetType(DfmEntryHandle_t xEntryHandle, uint16_t* pusType)
{
	DfmEntryHeader_t* pxEntryHeader;

	if (pxDfmEntryData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmEntryData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (xEntryHandle == 0)
	{
		return DFM_FAIL;
	}

	if (pusType == (void*)0)
	{
		return DFM_FAIL;
	}

	pxEntryHeader = (DfmEntryHeader_t*)xEntryHandle;

	*pusType = pxEntryHeader->usType;

	return DFM_SUCCESS;
}

DfmResult_t xDfmEntryGetEntryId(DfmEntryHandle_t xEntryHandle, uint16_t* pusEntryId)
{
	DfmEntryHeader_t* pxEntryHeader;

	if (pxDfmEntryData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmEntryData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (xEntryHandle == 0)
	{
		return DFM_FAIL;
	}

	if (pusEntryId == (void*)0)
	{
		return DFM_FAIL;
	}

	pxEntryHeader = (DfmEntryHeader_t*)xEntryHandle;

	*pusEntryId = pxEntryHeader->usEntryId;

	return DFM_SUCCESS;
}

DfmResult_t xDfmEntryGetChunkIndex(DfmEntryHandle_t xEntryHandle, uint16_t* pusChunkIndex)
{
	DfmEntryHeader_t* pxEntryHeader;

	if (pxDfmEntryData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmEntryData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (xEntryHandle == 0)
	{
		return DFM_FAIL;
	}

	if (pusChunkIndex == (void*)0)
	{
		return DFM_FAIL;
	}

	pxEntryHeader = (DfmEntryHeader_t*)xEntryHandle;

	*pusChunkIndex = pxEntryHeader->usChunkIndex;

	return DFM_SUCCESS;
}

DfmResult_t xDfmEntryGetChunkCount(DfmEntryHandle_t xEntryHandle, uint16_t* pusChunkCount)
{
	DfmEntryHeader_t* pxEntryHeader;

	if (pxDfmEntryData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmEntryData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (xEntryHandle == 0)
	{
		return DFM_FAIL;
	}

	if (pusChunkCount == (void*)0)
	{
		return DFM_FAIL;
	}

	pxEntryHeader = (DfmEntryHeader_t*)xEntryHandle;

	*pusChunkCount = pxEntryHeader->usChunkCount;

	return DFM_SUCCESS;
}

DfmResult_t xDfmEntryGetSessionIdSize(DfmEntryHandle_t xEntryHandle, uint16_t* pusSize)
{
	DfmEntryHeader_t* pxEntryHeader;

	if (pxDfmEntryData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmEntryData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (xEntryHandle == 0)
	{
		return DFM_FAIL;
	}

	if (pusSize == (void*)0)
	{
		return DFM_FAIL;
	}

	pxEntryHeader = (DfmEntryHeader_t*)xEntryHandle;

	*pusSize = pxEntryHeader->usSessionIdSize;

	return DFM_SUCCESS;
}

DfmResult_t xDfmEntryGetDeviceNameSize(DfmEntryHandle_t xEntryHandle, uint16_t* pusSize)
{
	DfmEntryHeader_t* pxEntryHeader;

	if (pxDfmEntryData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmEntryData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (xEntryHandle == 0)
	{
		return DFM_FAIL;
	}

	if (pusSize == (void*)0)
	{
		return DFM_FAIL;
	}

	pxEntryHeader = (DfmEntryHeader_t*)xEntryHandle;

	*pusSize = pxEntryHeader->usDeviceNameSize;

	return DFM_SUCCESS;
}

DfmResult_t xDfmEntryGetDescriptionSize(DfmEntryHandle_t xEntryHandle, uint16_t* pusSize)
{
	DfmEntryHeader_t* pxEntryHeader;

	if (pxDfmEntryData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmEntryData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (xEntryHandle == 0)
	{
		return DFM_FAIL;
	}

	if (pusSize == (void*)0)
	{
		return DFM_FAIL;
	}

	pxEntryHeader = (DfmEntryHeader_t*)xEntryHandle;

	*pusSize = pxEntryHeader->usDescriptionSize;

	return DFM_SUCCESS;
}

DfmResult_t xDfmEntryGetDataSize(DfmEntryHandle_t xEntryHandle, uint32_t* pulSize)
{
	DfmEntryHeader_t* pxEntryHeader;

	if (pxDfmEntryData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmEntryData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (xEntryHandle == 0)
	{
		return DFM_FAIL;
	}

	if (pulSize == (void*)0)
	{
		return DFM_FAIL;
	}

	pxEntryHeader = (DfmEntryHeader_t*)xEntryHandle;

	*pulSize = pxEntryHeader->ulDataSize;

	return DFM_SUCCESS;
}

DfmResult_t xDfmEntryGetAlertId(DfmEntryHandle_t xEntryHandle, uint32_t* pulAlertId)
{
	DfmEntryHeader_t* pxEntryHeader;

	if (pxDfmEntryData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmEntryData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (xEntryHandle == 0)
	{
		return DFM_FAIL;
	}

	if (pulAlertId == (void*)0)
	{
		return DFM_FAIL;
	}

	pxEntryHeader = (DfmEntryHeader_t*)xEntryHandle;

	*pulAlertId = pxEntryHeader->ulAlertId;

	return DFM_SUCCESS;
}

DfmResult_t xDfmEntryGetSessionId(DfmEntryHandle_t xEntryHandle, const char** pszSessionId)
{
	DfmEntryHeader_t* pxEntryHeader;

	if (pxDfmEntryData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmEntryData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (xEntryHandle == 0)
	{
		return DFM_FAIL;
	}

	if (pszSessionId == (void*)0)
	{
		return DFM_FAIL;
	}

	pxEntryHeader = (DfmEntryHeader_t*)xEntryHandle;

	/* TODO: 64-bit support */
	/* SessionId is located right after Header */
	*pszSessionId = (char*)((uint32_t)pxEntryHeader + sizeof(DfmEntryHeader_t)); /*cstat !MISRAC2012-Rule-18.4 The Session Id is stored after the Entry Header*/

	return DFM_SUCCESS;
}

DfmResult_t xDfmEntryGetDeviceName(DfmEntryHandle_t xEntryHandle, const char** pszDeviceName)
{
	DfmEntryHeader_t* pxEntryHeader;

	if (pxDfmEntryData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmEntryData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (xEntryHandle == 0)
	{
		return DFM_FAIL;
	}

	if (pszDeviceName == (void*)0)
	{
		return DFM_FAIL;
	}

	pxEntryHeader = (DfmEntryHeader_t*)xEntryHandle;

	/* TODO: 64-bit support. Also check alignment of all sizes. */
	/* Description is located right after SessionId */
	*pszDeviceName = (char*)((uint32_t)pxEntryHeader + sizeof(DfmEntryHeader_t) + (uint32_t)pxEntryHeader->usSessionIdSize); /*cstat !MISRAC2012-Rule-18.4 The Device Name is stored after the Entry Header and Session Id*/

	return DFM_SUCCESS;
}

DfmResult_t xDfmEntryGetDescription(DfmEntryHandle_t xEntryHandle, const char** pszDescription)
{
	DfmEntryHeader_t* pxEntryHeader;

	if (pxDfmEntryData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmEntryData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (xEntryHandle == 0)
	{
		return DFM_FAIL;
	}

	if (pszDescription == (void*)0)
	{
		return DFM_FAIL;
	}

	pxEntryHeader = (DfmEntryHeader_t*)xEntryHandle;

	/* TODO: 64-bit support. Also check alignment of all sizes. */
	/* Description is located right after SessionId */
	*pszDescription = (char*)((uint32_t)pxEntryHeader + sizeof(DfmEntryHeader_t) + (uint32_t)pxEntryHeader->usSessionIdSize + (uint32_t)pxEntryHeader->usDeviceNameSize); /*cstat !MISRAC2012-Rule-18.4 The Description is stored after the Entry Header, Session Id and Device Name*/

	return DFM_SUCCESS;
}

DfmResult_t xDfmEntryGetData(DfmEntryHandle_t xEntryHandle, void** ppvData)
{
	DfmEntryHeader_t* pxEntryHeader;

	if (pxDfmEntryData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmEntryData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (xEntryHandle == 0)
	{
		return DFM_FAIL;
	}

	if (ppvData == (void*)0)
	{
		return DFM_FAIL;
	}

	pxEntryHeader = (DfmEntryHeader_t*)xEntryHandle;

	/* TODO: 64-bit support. Also check alignment of all sizes. */
	/* Data is located right after Description */
	*ppvData = (void*)((uint32_t)pxEntryHeader + sizeof(DfmEntryHeader_t) + (uint32_t)pxEntryHeader->usSessionIdSize + (uint32_t)pxEntryHeader->usDeviceNameSize + (uint32_t)pxEntryHeader->usDescriptionSize); /*cstat !MISRAC2012-Rule-11.6 !MISRAC2012-Rule-18.4 The Entry Data is stored after the Entry Header, Session Id, Device Name and Description*/

	return DFM_SUCCESS;
}

DfmResult_t xDfmEntryGetEndMarkers(DfmEntryHandle_t xEntryHandle, uint8_t** pucMarkersBuffer)
{
	DfmEntryHeader_t* pxEntryHeader;

	if (pxDfmEntryData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmEntryData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (xEntryHandle == 0)
	{
		return DFM_FAIL;
	}

	if (pucMarkersBuffer == (void*)0)
	{
		return DFM_FAIL;
	}

	pxEntryHeader = (DfmEntryHeader_t*)xEntryHandle;

	if ((pxEntryHeader->cStartMarkers[0] != (uint16_t)0xD1) ||
		(pxEntryHeader->cStartMarkers[1] != (uint16_t)0xD2) ||
		(pxEntryHeader->cStartMarkers[2] != (uint16_t)0xD3) ||
		(pxEntryHeader->cStartMarkers[3] != (uint16_t)0xD4))
	{
		return DFM_FAIL;
	}

	/* TODO: 64-bit support. Also check alignment of all sizes. */
	/* Footer is located right after Data */
	*pucMarkersBuffer = (uint8_t*)((uint32_t)pxEntryHeader + sizeof(DfmEntryHeader_t) + (uint32_t)pxEntryHeader->usSessionIdSize + (uint32_t)pxEntryHeader->usDeviceNameSize + (uint32_t)pxEntryHeader->usDescriptionSize + pxEntryHeader->ulDataSize); /*cstat !MISRAC2012-Rule-18.4 The End Markers are stored after the Entry Header, Session Id, Device Name, Description and Entry Data*/

	return DFM_SUCCESS;
}

#endif
