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
 * @brief DFM FreeRTOS Kernel port API
 */

#ifndef DFM_KERNEL_PORT_H
#define DFM_KERNEL_PORT_H

#include <dfmConfig.h>

#if ((DFM_CFG_ENABLED) == 1)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup dfm_kernel_port_freertos_apis DFM FreeRTOS Kernel port API
 * @ingroup dfm_apis
 * @{
 */

/**
 * @brief Kernel port system data
 */
typedef struct DfmKernelPortData
{
	uint32_t dummy;
} DfmKernelPortData_t;

/**
 * @brief Initialize Kernel port system
 *
 * @param[in] pxBuffer Kernel port system buffer.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmKernelPortInitialize(DfmKernelPortData_t* pxBuffer);

/**
 * @brief Retrieves the current task
 *
 * @param[in] ppvTask Pointer where current task will be written.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmKernelPortGetCurrentTaskName(char** pszTaskName);

/** @} */

#ifdef __cplusplus
}
#endif

#endif

#endif
