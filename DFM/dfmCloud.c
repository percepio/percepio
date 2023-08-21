/*
 * Percepio DFM v2.0.0
 * Copyright 2023 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * DFM Cloud
 */

#include <dfm.h>
#include <stdio.h> /*cstat !MISRAC2012-Rule-21.6 We require snprintf() in order for the helper function to construct an MQTT topic*/

#if ((DFM_CFG_ENABLED) >= 1)

static DfmCloudData_t* pxCloudData = (void*)0;

DfmResult_t xDfmCloudInitialize(DfmCloudData_t* pxBuffer)
{
	if (pxBuffer == (void*)0)
	{
		return DFM_FAIL;
	}

	if (xDfmCloudPortInitialize(&pxBuffer->xCloudPortData) == DFM_FAIL)
	{
		return DFM_FAIL;
	}

	pxCloudData = pxBuffer;

	pxCloudData->ulInitialized = 1;

	return DFM_SUCCESS;
}

DfmResult_t xDfmCloudSendAlert(DfmEntryHandle_t xEntryHandle)
{
	DfmCloudStrategy_t xCloudStrategy = DFM_CLOUD_STRATEGY_OFFLINE;

	if (pxCloudData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxCloudData->ulInitialized == (uint32_t)0)
	{
		return DFM_FAIL;
	}

	if (ulDfmSessionIsEnabled() == (uint32_t)0)
	{
		return DFM_FAIL;
	}

	if (xEntryHandle == 0)
	{
		return DFM_FAIL;
	}

	if (xDfmSessionGetCloudStrategy(&xCloudStrategy) == DFM_FAIL)
	{
		return DFM_FAIL;
	}

	if (xCloudStrategy == DFM_CLOUD_STRATEGY_OFFLINE)
	{
		return DFM_FAIL;
	}

	return xDfmCloudPortSendAlert(xEntryHandle);
}

DfmResult_t xDfmCloudSendPayloadChunk(DfmEntryHandle_t xEntryHandle)
{
	DfmCloudStrategy_t xCloudStrategy = DFM_CLOUD_STRATEGY_OFFLINE;

	if (pxCloudData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxCloudData->ulInitialized == (uint32_t)0)
	{
		return DFM_FAIL;
	}

	if (ulDfmSessionIsEnabled() == (uint32_t)0)
	{
		return DFM_FAIL;
	}

	if (xEntryHandle == 0)
	{
		return DFM_FAIL;
	}

	if (xDfmSessionGetCloudStrategy(&xCloudStrategy) == DFM_FAIL)
	{
		return DFM_FAIL;
	}

	if (xCloudStrategy == DFM_CLOUD_STRATEGY_OFFLINE)
	{
		return DFM_FAIL;
	}

	return xDfmCloudPortSendPayloadChunk(xEntryHandle);
}

DfmResult_t xDfmCloudGenerateMQTTTopic(char* cTopicBuffer, uint32_t ulBufferSize, const char* szMQTTPrefix, DfmEntryHandle_t xEntryHandle)
{
	const char* szSessionId = (void*)0;
	const char* szDeviceName = (void*)0;
	uint32_t ulAlertId = (uint32_t)0;
	uint16_t usEntryId = (uint16_t)0;
	uint16_t usType = (uint16_t)0;
	uint16_t usChunkIndex = (uint16_t)0;
	uint16_t usChunkCount = (uint16_t)0;
	int32_t lRetVal;

	if (pxCloudData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxCloudData->ulInitialized == (uint32_t)0)
	{
		return DFM_FAIL;
	}

	if (cTopicBuffer == (void*)0)
	{
		return DFM_FAIL;
	}

	if (ulBufferSize == (uint32_t)0)
	{
		return DFM_FAIL;
	}

	if (xEntryHandle == 0)
	{
		return DFM_FAIL;
	}

	if (xDfmEntryGetSessionId(xEntryHandle, &szSessionId) == DFM_FAIL)
	{
		return DFM_FAIL;
	}

	if (xDfmEntryGetDeviceName(xEntryHandle, &szDeviceName) == DFM_FAIL)
	{
		return DFM_FAIL;
	}

	if (xDfmEntryGetAlertId(xEntryHandle, &ulAlertId) == DFM_FAIL)
	{
		return DFM_FAIL;
	}

	if (xDfmEntryGetType(xEntryHandle, &usType) == DFM_FAIL)
	{
		return DFM_FAIL;
	}

	if (xDfmEntryGetEntryId(xEntryHandle, &usEntryId) == DFM_FAIL)
	{
		return DFM_FAIL;
	}

	if (xDfmEntryGetChunkIndex(xEntryHandle, &usChunkIndex) == DFM_FAIL)
	{
		return DFM_FAIL;
	}

	if (xDfmEntryGetChunkCount(xEntryHandle, &usChunkCount) == DFM_FAIL)
	{
		return DFM_FAIL;
	}

	if (szMQTTPrefix == (void*)0)
	{
		/* Set it to empty */
		szMQTTPrefix = "";
	}

	/* "<PREFIX>DevAlert/<DEVICE_NAME>/<UNIQUE_SESSION_ID>/<TRACE_COUNTER>/<SLICE_ID>-<TOTAL_EXPECTED_SLICES>_<PAYLOAD_TYPE>" */
	switch (usType)
	{
	case DFM_ENTRY_TYPE_ALERT:
		lRetVal = snprintf(cTopicBuffer, ulBufferSize, "%sDevAlert/%s/%s/%ld/%d-%d_da_header", szMQTTPrefix, szDeviceName, szSessionId, ulAlertId, usChunkIndex, usChunkCount);
		break;
	case DFM_ENTRY_TYPE_PAYLOAD_HEADER:
		lRetVal = snprintf(cTopicBuffer, ulBufferSize, "%sDevAlert/%s/%s/%ld/%d-%d_da_payload%d_header", szMQTTPrefix, szDeviceName, szSessionId, ulAlertId, usChunkIndex, usChunkCount, usEntryId);
		break;
	case DFM_ENTRY_TYPE_PAYLOAD:
		lRetVal = snprintf(cTopicBuffer, ulBufferSize, "%sDevAlert/%s/%s/%ld/%d-%d_da_payload%d", szMQTTPrefix, szDeviceName, szSessionId, ulAlertId, usChunkIndex, usChunkCount, usEntryId);
		break;
	default:
		return DFM_FAIL;
		break;
	}

	/* TODO: Instead of doing an strlen after calling this function, we should reuse this value (pass a pointer to a len var or similar */
	if ((lRetVal < (int32_t)0) || (lRetVal >= (int32_t)ulBufferSize))
	{
		return DFM_FAIL;
	}

	return DFM_SUCCESS;
}

#endif
