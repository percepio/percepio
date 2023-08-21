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
 * @brief DFM dummy Cloud port API
 */

#ifndef DFM_CLOUD_PORT_H
#define DFM_CLOUD_PORT_H

#include <stdint.h>
#include <dfmTypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup dfm_cloud_port_dummy_apis DFM dummy Cloud port API
 * @ingroup dfm_apis
 * @{
 */

/**
 * @brief Cloud port system data
 */
typedef struct DfmCloudPortData
{
	uint32_t dummy;
} DfmCloudPortData_t;

/**
 * @brief Initialize Cloud port system
 *
 * @param[in] pxBuffer Cloud port system buffer.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmCloudPortInitialize(DfmCloudPortData_t* pxBuffer);

/**
 * @brief Send Alert Entry
 *
 * @param[in] xEntryHandle Entry handle.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmCloudPortSendAlert(DfmEntryHandle_t xEntryHandle);

/**
 * @brief Send Payload chunk Entry
 *
 * @param[in] xEntryHandle Entry handle.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmCloudPortSendPayloadChunk(DfmEntryHandle_t xEntryHandle);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
