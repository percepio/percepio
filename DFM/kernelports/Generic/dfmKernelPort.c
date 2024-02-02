/*
 * Percepio DFM v2.1.0
 * Copyright 2023 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Generic Kernel port
 */

#include <dfm.h>
#include <dfmKernelPort.h>

#if ((DFM_CFG_ENABLED) >= 1)

DfmResult_t xDfmKernelPortInitialize(DfmKernelPortData_t *pxBuffer)
{
	(void)pxBuffer;

	return DFM_SUCCESS;
}

DfmResult_t xDfmKernelPortGetCurrentTaskName(char** pszTaskName)
{
	if (pszTaskName == (void*)0)
	{
		return DFM_FAIL;
	}

	*pszTaskName = "UnknownTask";	/* This can be changed to retrieve the current task name if a kernel is present */

	return DFM_SUCCESS;
}

#endif
