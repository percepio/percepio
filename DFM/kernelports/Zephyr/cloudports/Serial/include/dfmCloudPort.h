/*
 * Percepio DFM v2.0.0
 * Copyright 2023 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DFM_CLOUD_PORT_H
#define DFM_CLOUD_PORT_H

#include <stdint.h>
#include <dfmTypes.h>
#include <dfmCloudPortConfig.h>

#ifdef __cplusplus
extern "C" {
#endif

/* This will allow DFM to attempt transfers in all situations, hardfaults included */
#define DFM_CLOUD_PORT_ALWAYS_ATTEMPT_TRANSFER

#if CONFIG_PERCEPIO_DFM_CFG_CLOUDPORT_SERIAL == 1

typedef struct {
	uint32_t startmarker;
	uint16_t keylen;
	uint16_t datalen;
} DfmSerialHeader_t;

/**
* @brief Cloud port specific data, additional header data needed to make sure the message ends up
 * 		 where it's supposed to.
*/
typedef struct DfmCloudPortData
{
	char buf[80];
	char cKeyBuffer[DFM_CFG_CLOUD_PORT_MAX_TOPIC_SIZE];
	DfmSerialHeader_t xDfmSerialHeader;
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

#endif //PERCEPIO_DFM_CFG_CLOUDPORT_SERIAL == 1

#ifdef __cplusplus
}
#endif

#endif //DFM_CLOUD_PORT_H
