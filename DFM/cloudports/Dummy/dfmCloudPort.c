/*
 * Percepio DFM v2.1.0
 * Copyright 2023 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * DFM Dummy cloud port implementation that refuses to send anything
 */

#include <dfm.h>
#include <dfmCloudPort.h>

#if ((DFM_CFG_ENABLED) >= 1)

DfmResult_t xDfmCloudPortInitialize(DfmCloudPortData_t* pxBuffer)
{
	(void)pxBuffer;

	return DFM_SUCCESS;
}

DfmResult_t xDfmCloudPortSendAlert(DfmEntryHandle_t xEntryHandle)
{
	(void)xEntryHandle;

	return DFM_FAIL;
}

DfmResult_t xDfmCloudPortSendPayloadChunk(DfmEntryHandle_t xEntryHandle)
{
	(void)xEntryHandle;

	return DFM_FAIL;
}

#endif
