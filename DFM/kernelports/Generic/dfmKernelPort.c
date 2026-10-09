/*
 * Percepio DFM
 * Copyright 2023-2026 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * BareMetal Kernel port
 */

#include <dfm.h>

#if ((DFM_CFG_ENABLED) >= 1)

static DfmKernelPortData_t *pxKernelPortData = (void*)0;

DfmResult_t xDfmKernelPortInitialize(DfmKernelPortData_t *pxBuffer)
{
	if (pxBuffer == (void*)0)
	{
		return DFM_FAIL;
	}

	pxKernelPortData = pxBuffer;

	return DFM_SUCCESS;
}

DfmResult_t xDfmKernelPortGetCurrentTaskName(char** pszTaskName)
{
	*pszTaskName = "N/A";

	return DFM_SUCCESS;
}

#endif
