/*
 * Percepio DFM
 * Copyright 2023-2026 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * FreeRTOS Kernel port
 */

#include <dfm.h>

#include <FreeRTOS.h>
#include <task.h>

#if ((DFM_CFG_ENABLED) >= 1)

static DfmKernelPortData_t *pxKernelPortData = (void*)0;

DfmResult_t xDfmKernelPortInitialize(DfmKernelPortData_t *pxBuffer)
{
	if (pxBuffer == (void*)0)
	{
		return DFM_FAIL;
	}

	pxKernelPortData = pxBuffer;

	// All DFM submodules have their own data storage object, allocated in
	// the main DFM datastructure. Not used here, but might be needed 
	// in the future. Assign the dummy field to avoid warnings.
	pxKernelPortData->dummy = 0;
        
	return DFM_SUCCESS;
}

DfmResult_t xDfmKernelPortGetCurrentTaskName(char** pszTaskName)
{
	TaskHandle_t xCurrentTaskHandle = 0;
	
	if (pszTaskName == (void*)0)
	{
		return DFM_FAIL;
	}
	
	xCurrentTaskHandle = xTaskGetCurrentTaskHandle();
	
	if (xCurrentTaskHandle == 0)
	{
		return DFM_FAIL;
	}

	*pszTaskName = pcTaskGetName(xCurrentTaskHandle);

	return DFM_SUCCESS;
}

#endif
