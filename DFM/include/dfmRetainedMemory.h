/*
 * Percepio DFM v2.1.0
 * Copyright 2023 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief DFM Retained Memory API
 */

#ifndef DFM_RETAINED_MEMORY_H
#define DFM_RETAINED_MEMORY_H

#include <stdint.h>
#include <dfmConfig.h>
#include <dfmTypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DFM_CFG_RETAINED_MEMORY
#define DFM_CFG_RETAINED_MEMORY 0
#endif

#if (defined(DFM_CFG_ENABLED) && (DFM_CFG_ENABLED >= 1)) && (defined(DFM_CFG_RETAINED_MEMORY) && (DFM_CFG_RETAINED_MEMORY >= 1))

#include <dfmRetainedMemoryPort.h>

/**
 * @defgroup dfm_alert_apis DFM Alert API
 * @ingroup dfm_apis
 * @{
 */

typedef struct DfmRetainedMemoryData
{
	uint32_t ulInitialized;
	uint32_t ulWriteOffset;
	uint32_t ulReadOffset;
	uint32_t alignment;
	DfmRetainedMemoryPortData_t xRetainedMemoryPortData;
} DfmRetainedMemoryData_t;

/**
 * @brief Initialize Retained Memory system
 *
 * @param[in] pxBuffer Retained Memory system buffer.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmRetainedMemoryInitialize(DfmRetainedMemoryData_t *pxBuffer);

/**
 * @brief Write Alert Entry
 *
 * @param[in] xEntryHandle Entry handle.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmRetainedMemoryWriteAlert(DfmEntryHandle_t xEntryHandle);

/**
 * @brief Read Alert Entry
 *
 * @param[in] pvBuffer Pointer to Alert Entry buffer.
 * @param[in] ulBufferSize Alert Entry buffer size.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmRetainedMemoryReadAlert(void* pvBuffer, uint32_t ulBufferSize);

/**
 * @brief Write Payload chunk Entry
 *
 * @param[in] xEntryHandle Entry handle.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmRetainedMemoryWritePayloadChunk(DfmEntryHandle_t xEntryHandle);

/**
 * @brief Read Payload chunk Entry
 *
 * @param[in] szSessionId Requested Session Id.
 * @param[in] ulAlertId Requested Alert Id.
 * @param[in] pvBuffer Pointer to Payload chunk Entry buffer.
 * @param[in] ulBufferSize Payload chunk Entry buffer size.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmRetainedMemoryReadPayloadChunk(char* szSessionId, uint32_t ulAlertId, void* pvBuffer, uint32_t ulBufferSize);

/**
 * @brief Clear all stored alerts from Retained Memory
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmRetainedMemoryClear(void);

/** @} */

#else

#define xDfmRetainedMemoryInitialize(pxBuffer) ((void)(pxBuffer), DFM_SUCCESS)
#define xDfmRetainedMemoryWriteAlert(xEntryHandle, ulOverwrite) ((void)(xEntryHandle), (void)(ulOverwrite), DFM_FAIL)
#define xDfmRetainedMemoryReadAlert(pvBuffer, ulBufferSize) ((void)(pvBuffer), (void)(ulBufferSize), DFM_FAIL)
#define xDfmRetainedMemoryWritePayloadChunk(xEntryHandle, ulOverwrite) ((void)(xEntryHandle), (void)(ulOverwrite), DFM_FAIL)
#define xDfmRetainedMemoryReadPayloadChunk(szSessionId, ulAlertId, pvBuffer, ulBufferSize) ((void)(szSessionId), (void)(ulAlertId), (void)(pvBuffer), (void)(ulBufferSize), DFM_FAIL)
#define xDfmRetainedMemoryReset(void) (DFM_SUCCESS)

#endif

#ifdef __cplusplus
}
#endif

#endif
