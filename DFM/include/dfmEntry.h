/*
 * Percepio DFM v2.0.0
 * Copyright 2023 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief DFM Entry
 */

#ifndef DFM_ENTRY_H
#define DFM_ENTRY_H

#include <dfmTypes.h>
#include <dfmConfig.h>

#ifdef __cplusplus
extern "C" {
#endif

#if ((DFM_CFG_ENABLED) >= 1)

/**
 * @defgroup dfm_entry_api DFM Entry API
 * @ingroup dfm_apis
 * @{
 */


/**
 * @brief Entry system data
 */
typedef struct DfmEntryData
{
	uint32_t ulInitialized;
	uint8_t buffer[(uint32_t)(DFM_CFG_MAX_PAYLOAD_CHUNK_SIZE) + 128UL]; /* We don't set it to exact size since we might need to read old/new DFM Entry versions that don't match exactly */
} DfmEntryData_t;

/**
 * @internal Initializes the Entry system
 *
 * @param[in] pxBuffer Entry system buffer.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmEntryInitialize(DfmEntryData_t* pxBuffer);

/**
 * @brief Get the Entry system buffer pointer and size.
 *
 * @param[out] ppvBuffer Pointer to buffer.
 * @param[out] pulBufferSize Buffer size.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmEntryGetBuffer(void** ppvBuffer, uint32_t* pulBufferSize);

/**
 * @brief Create an Alert Entry from Alert handle.
 *
 * @param[in] xAlertHandle Alert handle.
 * @param[out] pxEntryHandle Pointer to Entry handle.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmEntryCreateAlert(DfmAlertHandle_t xAlertHandle, DfmEntryHandle_t *pxEntryHandle);

/**
 * @brief Create an Payload Header Entry from Alert handle and Payload information.
 *
 * @param[in] xAlertHandle Alert handle.
 * @param[in] usEntryId Entry Id.
 * @param[in] ulPayloadSize Payload size.
 * @param[in] szDescription Payload description.
 * @param[out] pxEntryHandle Pointer to Entry handle.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmEntryCreatePayloadHeader(DfmAlertHandle_t xAlertHandle, uint16_t usEntryId, uint32_t ulPayloadSize, char* szDescription, DfmEntryHandle_t* pxEntryHandle);

/**
 * @brief Create an Payload chunk Entry from Alert handle and Payload chunk information.
 *
 * @param[in] xAlertHandle Alert handle.
 * @param[in] usEntryId Entry Id.
 * @param[in] usChunkIndex This chunk's index.
 * @param[in] usChunkCount Payload's total chunk count.
 * @param[in] pvPayload Pointer to Payload chunk.
 * @param[in] ulSize Payload chunk size.
 * @param[in] szDescription Payload description.
 * @param[out] pxEntryHandle Pointer to Entry handle.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmEntryCreatePayloadChunk(DfmAlertHandle_t xAlertHandle, uint16_t usEntryId, uint16_t usChunkIndex, uint16_t usChunkCount, void* pvPayload, uint32_t ulSize, char* szDescription, DfmEntryHandle_t* pxEntryHandle);

/**
 * @brief Create an Alert Entry from the Entry system buffer.
 *
 * @param[out] pxEntryHandle Pointer to Entry handle.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmEntryCreateAlertFromBuffer(DfmEntryHandle_t* pxEntryHandle);

/**
 * @brief Create a Payload chunk Entry from the Entry system buffer.
 *
 * @param[in] szSessionId Session Id.
 * @param[in] ulAlertId Alert Id.
 * @param[out] pxEntryHandle Pointer to Entry handle.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmEntryCreatePayloadChunkFromBuffer(const char* szSessionId, uint32_t ulAlertId, DfmEntryHandle_t* pxEntryHandle);

/**
 * @brief Get the Entry size from Entry handle.
 *
 * @param[in] xEntryHandle Entry handle.
 * @param[out] pulSize Entry size.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmEntryGetSize(DfmEntryHandle_t xEntryHandle, uint32_t* pulSize);

/**
 * @brief Get the start markers from Entry handle.
 *
 * @param[in] xEntryHandle Entry handle.
 * @param[out] pucMarkersBuffer Pointer to start markers.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmEntryGetStartMarkers(DfmEntryHandle_t xEntryHandle, uint8_t** pucMarkersBuffer);

/**
 * @brief Get the endianess from Entry handle.
 *
 * @param[in] xEntryHandle Entry handle.
 * @param[out] pusEndianess Pointer to endianess.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmEntryGetEndianess(DfmEntryHandle_t xEntryHandle, uint16_t* pusEndianess);

/**
 * @brief Get the version from Entry handle.
 *
 * @param[in] xEntryHandle Entry handle.
 * @param[out] pusVersion Pointer to version.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmEntryGetVersion(DfmEntryHandle_t xEntryHandle, uint16_t* pusVersion);

/**
 * @brief Get the Entry type from Entry handle.
 *
 * @param[in] xEntryHandle Entry handle.
 * @param[out] pusType Pointer to Entry type.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmEntryGetType(DfmEntryHandle_t xEntryHandle, uint16_t* pusType);

/**
 * @brief Get the Entry Id from Entry handle.
 *
 * @param[in] xEntryHandle Entry handle.
 * @param[out] pusEntryId Pointer to Entry Id.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmEntryGetEntryId(DfmEntryHandle_t xEntryHandle, uint16_t* pusEntryId);

/**
 * @brief Get the chunk index from Entry handle.
 *
 * @param[in] xEntryHandle Entry handle.
 * @param[out] pusChunkIndex Pointer to chunk index.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmEntryGetChunkIndex(DfmEntryHandle_t xEntryHandle, uint16_t* pusChunkIndex);

/**
 * @brief Get the chunk count from Entry handle.
 *
 * @param[in] xEntryHandle Entry handle.
 * @param[out] pusChunkCount Pointer to chunk count.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmEntryGetChunkCount(DfmEntryHandle_t xEntryHandle, uint16_t* pusChunkCount);

/**
 * @brief Get the Session Id size from Entry handle.
 *
 * @param[in] xEntryHandle Entry handle.
 * @param[out] pusSize Pointer to Session Id size.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmEntryGetSessionIdSize(DfmEntryHandle_t xEntryHandle, uint16_t* pusSize);

/**
 * @brief Get the Device Name size from Entry handle.
 *
 * @param[in] xEntryHandle Entry handle.
 * @param[out] pusSize Pointer to Device Name size.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmEntryGetDeviceNameSize(DfmEntryHandle_t xEntryHandle, uint16_t* pusSize);

/**
 * @brief Get the description size from Entry handle.
 *
 * @param[in] xEntryHandle Entry handle.
 * @param[out] pusSize Pointer to description size.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmEntryGetDescriptionSize(DfmEntryHandle_t xEntryHandle, uint16_t* pusSize);

/**
 * @brief Get the data size from Entry handle.
 *
 * @param[in] xEntryHandle Entry handle.
 * @param[out] pulSize Pointer to data size.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmEntryGetDataSize(DfmEntryHandle_t xEntryHandle, uint32_t* pulSize);

/**
 * @brief Get the Alert Id from Entry handle.
 *
 * @param[in] xEntryHandle Entry handle.
 * @param[out] pulAlertId Pointer to Alert Id.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmEntryGetAlertId(DfmEntryHandle_t xEntryHandle, uint32_t* pulAlertId);

/**
 * @brief Get the Session Id from Entry handle
 *
 * @param[in] xEntryHandle Entry handle.
 * @param[out] pszSessionId Pointer to Session Id.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmEntryGetSessionId(DfmEntryHandle_t xEntryHandle, const char** pszSessionId);

/**
 * @brief Get the Device Name from Entry handle
 *
 * @param[in] xEntryHandle Entry handle.
 * @param[out] pszDeviceName Pointer to Device Name.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmEntryGetDeviceName(DfmEntryHandle_t xEntryHandle, const char** pszDeviceName);

/**
 * @brief Get the description from Entry handle.
 *
 * @param[in] xEntryHandle Entry handle.
 * @param[out] pszDescription Pointer to description.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmEntryGetDescription(DfmEntryHandle_t xEntryHandle, const char** pszDescription);

/**
 * @brief Get the data from Entry handle.
 *
 * @param[in] xEntryHandle Entry handle.
 * @param[out] ppvData Pointer to data.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmEntryGetData(DfmEntryHandle_t xEntryHandle, void** ppvData);

/**
 * @brief Get the end markers from Entry handle.
 *
 * @param[in] xEntryHandle Entry handle.
 * @param[out] pucMarkersBuffer Pointer to end markers.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmEntryGetEndMarkers(DfmEntryHandle_t xEntryHandle, uint8_t** pucMarkersBuffer);

/** @} */

#else

/* Dummy defines */
#define xDfmEntryGetBuffer(ppvBuffer, pulBufferSize) (DFM_FAIL)
#define xDfmEntryCreateAlert(xAlertHandle, pxEntryHandle) (DFM_FAIL)
#define xDfmEntryCreatePayloadHeader(xAlertHandle, usEntryId, ulPayloadSize, szDescription, pxEntryHandle) (DFM_FAIL)
#define xDfmEntryCreatePayloadChunk(xAlertHandle, usEntryId, usChunkIndex, usChunkCount, pvPayload, ulSize, szDescription, pxEntryHandle) (DFM_FAIL)
#define xDfmEntryCreateAlertFromBuffer(pxEntryHandle) (DFM_FAIL)
#define xDfmEntryCreatePayloadChunkFromBuffer(szSessionId, ulAlertId, pxEntryHandle) (DFM_FAIL)
#define xDfmEntryGetSize(xEntryHandle, pulSize) (DFM_FAIL)
#define xDfmEntryGetStartMarkers(xEntryHandle, pucMarkersBuffer) (DFM_FAIL)
#define xDfmEntryGetEndianess(xEntryHandle, pusEndianess) (DFM_FAIL)
#define xDfmEntryGetVersion(xEntryHandle, pusVersion) (DFM_FAIL)
#define xDfmEntryGetType(xEntryHandle, pusType) (DFM_FAIL)
#define xDfmEntryGetEntryId(xEntryHandle, pusEntryId) (DFM_FAIL)
#define xDfmEntryGetChunkIndex(xEntryHandle, pusChunkIndex) (DFM_FAIL)
#define xDfmEntryGetChunkCount(xEntryHandle, pusChunkCount) (DFM_FAIL)
#define xDfmEntryGetSessionIdSize(xEntryHandle, pusSize) (DFM_FAIL)
#define xDfmEntryGetDescriptionSize(xEntryHandle, pusSize) (DFM_FAIL)
#define xDfmEntryGetDataSize(xEntryHandle, pulSize) (DFM_FAIL)
#define xDfmEntryGetAlertId(xEntryHandle, pulAlertId) (DFM_FAIL)
#define xDfmEntryGetSessionId(xEntryHandle, pszSessionId) (DFM_FAIL)
#define xDfmEntryGetDescription(xEntryHandle, pszDescription) (DFM_FAIL)
#define xDfmEntryGetData(xEntryHandle, ppvData) (DFM_FAIL)
#define xDfmEntryGetEndMarkers(xEntryHandle, pucMarkersBuffer) (DFM_FAIL)

#endif

#ifdef __cplusplus
}
#endif

#endif
