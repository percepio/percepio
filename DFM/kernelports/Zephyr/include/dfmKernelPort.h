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
 * @brief DFM Zephyr Kernel port API
 */

#ifndef DFM_KERNEL_PORT_H
#define DFM_KERNEL_PORT_H

#include <dfmConfig.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>

#if ((DFM_CFG_ENABLED) == 1)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup dfm_kernel_port_zephyr_apis DFM Zephyr Kernel port API
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
 * @brief Retrieves the current task name
 *
 * @param[in] ppvTask Pointer where current task will be written.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmKernelPortGetCurrentTaskName(char** pszTaskName);

/** @} */

/**
 * Disable interrupts, kernel port specific
 */
#define vDfmDisableInterrupts() irq_lock();

#if DFM_CFG_ENABLE_COREDUMPS == 1
/**
 * Append a coredump to an alert. Where a normal coredump called from the kernel will result in a new alert being generated,
 * this is meant for alerts with a reason > 0xFFFF0000. When calling upon this function, the internal state stored within the
 * kernel port is appended to the alert specified (which could be either to be sent directly to the CloudPort or stored
 * by the StoragePort, depending on CloudPort availability and user settings).
 * @param xAlertHandle The alert which the coredump should be attached to
 * @return
 */
DfmResult_t xDfmAlertAddCoredump(DfmAlertHandle_t xAlertHandle);
#else
#define xDfmAlertAddCoredump(xAlertHandle) (DFM_FAIL)
#endif

#if defined(CONFIG_PERCEPIO_TRACERECORDER) && CONFIG_PERCEPIO_TRACERECORDER == 1 && defined(CONFIG_PERCEPIO_TRC_CFG_STREAM_PORT_RINGBUFFER)
/**
 * Attach a trace to the Alert. This requires the TraceAlyzer module to be enabled and configured to use the Ring Buffer.
 * Note that this function will disable tracing to be able to capture and attach the trace, thus, if you wish to have the
 * tracing continue after adding the trace to your alert, you will need to re-enable tracing bu executing xTraceEnable(TRC_START)
 * afterwards.
 *
 * @param xAlertHandle The alert which the trace should be attached to
 * @return
 */
DfmResult_t xDfmAlertAddTrace(DfmAlertHandle_t xAlertHandle);
#endif

#ifdef __cplusplus
}
#endif

#endif

#endif
