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
  * @brief DFM Cloud API
  */

#ifndef DFM_CLOUD_H
#define DFM_CLOUD_H

#include <dfmCloudPort.h>

#ifdef __cplusplus
extern "C" {
#endif

#if ((DFM_CFG_ENABLED) >= 1)

/**
 * @defgroup dfm_cloud_apis DFM Cloud API
 * @ingroup dfm_apis
 * @{
 */

/**
 * @brief Cloud system data
 */
typedef struct DfmCloudData
{
	uint32_t ulInitialized;
	DfmCloudPortData_t xCloudPortData;
} DfmCloudData_t;

/**
 * @internal Initializes the Cloud system
 *
 * @param[in] pxBuffer Cloud system buffer.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmCloudInitialize(DfmCloudData_t* pxBuffer);

/**
 * @brief Send an Alert
 *
 * @param[in] xEntryHandle Entry handle.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmCloudSendAlert(DfmEntryHandle_t xEntryHandle);

/**
 * @brief Send a Payload chunk
 *
 * @param[in] xEntryHandle Entry handle.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmCloudSendPayloadChunk(DfmEntryHandle_t xEntryHandle);

/**
 * @brief Helper function used by MQTT Cloud ports that generates a Topic from an Entry handle
 *
 * @param[in] cTopicBuffer Pointer to buffer.
 * @param[in] ulBufferSize Buffer size.
 * @param[in] szMQTTPrefix MQTT Topic Prefix.
 * @param[in] xEntryHandle Entry handle.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmCloudGenerateMQTTTopic(char* cTopicBuffer, uint32_t ulBufferSize, const char* szMQTTPrefix, DfmEntryHandle_t xEntryHandle);

/** @} */

#else

/* Dummy defines */
#define xDfmCloudSendAlert(xEntryHandle) (DFM_FAIL)
#define xDfmCloudSendPayloadChunk(xEntryHandle) (DFM_FAIL)
#define xDfmCloudGenerateMQTTTopic(cTopicBuffer, ulBufferSize, xEntryHandle) (DFM_FAIL)

#endif

#ifdef __cplusplus
}
#endif

#endif
