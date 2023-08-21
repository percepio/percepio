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
 * @brief DFM Storage API
 */

#ifndef DFM_STORAGE_H
#define DFM_STORAGE_H

#include <stdint.h>
#include <dfmStoragePort.h>

#ifdef __cplusplus
extern "C" {
#endif

#if ((DFM_CFG_ENABLED) >= 1)

/**
 * @defgroup dfm_storage_apis DFM Storage API
 * @ingroup dfm_apis
 * @{
 */

/**
 * @brief Storage data
 */
typedef struct DfmStorageData
{
	uint32_t ulInitialized;

	DfmStoragePortData_t xStoragePortData;
} DfmStorageData_t;

/**
 * @internal Initialize Storage system.
 *
 * @param[in] pxBuffer Pointer to memory that will be used by the storage system.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmStorageInitialize(DfmStorageData_t *pxBuffer);

/**
 * @brief Store Session data
 *
 * @param[in] pvSession Session data.
 * @param[in] ulSessionSize Data size.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmStorageStoreSession(void* pvSession, uint32_t ulSessionSize);

/**
* @brief Retrieve Session data
*
* @param[in] pvBuffer Pointer to buffer.
* @param[in] ulBufferSize Buffer size.
*
* @retval DFM_FAIL Failure
* @retval DFM_SUCCESS Success
*/
DfmResult_t xDfmStorageGetSession(void* pvBuffer, uint32_t ulBufferSize);

/**
* @brief Store Alert
*
* @param[in] xEntryHandle The Entry containing an Alert.
*
* @retval DFM_FAIL Failure
* @retval DFM_SUCCESS Success
*/
DfmResult_t xDfmStorageStoreAlert(DfmEntryHandle_t xEntryHandle);

/**
* @brief Retrieve Alert
*
* @param[in] pvBuffer Pointer to buffer.
* @param[in] ulBufferSize Buffer size.
*
* @retval DFM_FAIL Failure
* @retval DFM_SUCCESS Success
*/
DfmResult_t xDfmStorageGetAlert(void* pvBuffer, uint32_t ulBufferSize);

/**
* @brief Store Payload chunk
*
* @param[in] xEntryHandle The Entry containing a Payload chunk.
*
* @retval DFM_FAIL Failure
* @retval DFM_SUCCESS Success
*/
DfmResult_t xDfmStorageStorePayloadChunk(DfmEntryHandle_t xEntryHandle);

/**
* @brief Retrieve Payload chunk
*
* @param[in] pvBuffer Pointer to buffer.
* @param[in] ulBufferSize Buffer size.
*
* @retval DFM_FAIL Failure
* @retval DFM_SUCCESS Success
*/
DfmResult_t xDfmStorageGetPayloadChunk(char* szSessionId, uint32_t ulAlertId, void* pvBuffer, uint32_t ulBufferSize);

/** @} */

#else

/* Dummy defines */
#define xDfmStorageStoreSession(pvSession, ulSessionSize) (DFM_FAIL)
#define xDfmStorageGetSession(pvBuffer, ulBufferSize) (DFM_FAIL)
#define xDfmStorageStoreAlert(xEntryHandle) (DFM_FAIL)
#define xDfmStorageGetAlert(pvBuffer, ulBufferSize) (DFM_FAIL)
#define xDfmStorageStorePayloadChunk(xEntryHandle) (DFM_FAIL)
#define xDfmStorageGetPayloadChunk(szSessionId, ulAlertId, pvBuffer, ulBufferSize) (DFM_FAIL)

#endif

#ifdef __cplusplus
}
#endif

#endif
