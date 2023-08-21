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
 * @brief DFM Zephyr Flash Storage port API
 */

#ifndef DFM_STORAGE_PORT_H
#define DFM_STORAGE_PORT_H

#include <stdint.h>
#include <dfmConfig.h>
#include <dfmTypes.h>

#if (defined(DFM_CFG_ENABLED) && (DFM_CFG_ENABLED >= 1))

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup dfm_storage_port_zephyr_flash_apis DFM Zephyr Flash Storage port API
 * @ingroup dfm_apis
 * @{
 */

/**
 * @brief Storage port system data
 */
typedef struct DfmStoragePortData
{
	uint32_t ulInitialized;
} DfmStoragePortData_t;

/**
 * @brief Initialize Storage port system
 *
 * @param[in] pxBuffer Storage port system buffer.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmStoragePortInitialize(DfmStoragePortData_t *pxBuffer);

/**
 * @brief Store Session data
 *
 * @param[in] pvData Pointer to Session data.
 * @param[in] ulSize Session data size.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmStoragePortStoreSession(void* pvData, uint32_t ulSize);

/**
 * @brief Retrieve Session data
 *
 * @param[in] pvData Pointer to Session data buffer.
 * @param[in] ulSize Session data buffer size.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmStoragePortGetSession(void* pvBuffer, uint32_t ulBufferSize);

/**
 * @brief Store Alert Entry
 *
 * @param[in] xEntryHandle Entry handle.
 * @param[in] ulOverwrite Flag indicating if existing Entries should be overwritten.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmStoragePortStoreAlert(DfmEntryHandle_t xEntryHandle, uint32_t ulOverwrite);

/**
 * @brief Retrieve Alert Entry
 *
 * @param[in] pvBuffer Pointer to Alert Entry buffer.
 * @param[in] ulBufferSize Alert Entry buffer size.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmStoragePortGetAlert(void* pvBuffer, uint32_t ulBufferSize);

/**
 * @brief Store Payload chunk Entry
 *
 * @param[in] xEntryHandle Entry handle.
 * @param[in] ulOverwrite Flag indicating if existing Entries should be overwritten.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmStoragePortStorePayloadChunk(DfmEntryHandle_t xEntryHandle, uint32_t ulOverwrite);

/**
 * @brief Retrieve Payload chunk Entry
 *
 * @param[in] szSessionId Requested Session Id.
 * @param[in] ulAlertId Requested Alert Id.
 * @param[in] pvBuffer Pointer to Payload chunk Entry buffer.
 * @param[in] ulBufferSize Payload chunk Entry buffer size.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmStoragePortGetPayloadChunk(char* szSessionId, uint32_t ulAlertId, void* pvBuffer, uint32_t ulBufferSize);

/** @} */

#ifdef __cplusplus
}
#endif

#endif

#endif
