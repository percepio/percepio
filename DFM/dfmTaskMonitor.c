/*
 * Percepio DFM
 * Copyright 2023-2026 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * DFM task monitor
 */

#include <inttypes.h>
#include <string.h>
#include <dfm.h>
#include <dfmKernelPort.h>
#include <dfmUtility.h>

#if ((TRC_CFG_ENABLE_TASK_MONITOR) >= 1)

static void prvOnTaskAnomaly(TraceTaskMonitorCallbackData_t *pxData);

DfmResult_t xDfmTaskMonitorInit(void)
{
	return xDfmTaskMonitorSetCallback(prvOnTaskAnomaly);
}

static void prvOnTaskAnomaly(TraceTaskMonitorCallbackData_t *pxData)
{
	DfmAlertHandle_t xAlertHandle;
    void* pvBuffer = (void*)0;
    uint32_t ulBufferSize = 0;
    static TraceStringHandle_t TzUserEventChannel = 0;

	if (pxData->uxCPULoad > pxData->uxHighLimit)
	{
		snprintf(cDfmPrintBuffer, sizeof(cDfmPrintBuffer), "Task %s executed more than expected (%u %%).\n", pxData->acName, (unsigned int)pxData->uxCPULoad);
	}
	else if (pxData->uxCPULoad < pxData->uxLowLimit)
	{
		snprintf(cDfmPrintBuffer, sizeof(cDfmPrintBuffer), "Task %s executed less than expected (%u %%).\n", pxData->acName, (unsigned int)pxData->uxCPULoad);
	}
	else
	{
		/* Not supposed to happen... */
		snprintf(cDfmPrintBuffer, sizeof(cDfmPrintBuffer), "TaskMonitor alert, Task %s, CPU load %u %%, unknown reason.\n", pxData->acName, (unsigned int)pxData->uxCPULoad);
	}

    if (xDfmAlertBegin(DFM_TYPE_TASKMONITOR_ANOMALY, cDfmPrintBuffer, &xAlertHandle) != DFM_SUCCESS)
	{
		return;
	}

		if (TzUserEventChannel == 0)
		{
			(void)xTraceStringRegister("ALERT", &TzUserEventChannel);
		}

		(void)xTracePrint(TzUserEventChannel, cDfmPrintBuffer);

		/* Stopping the tracing while sending the trace data. */
		(void)xTraceDisable();

		(void)xTraceGetEventBuffer(&pvBuffer, &ulBufferSize);
		(void)xDfmAlertAddPayload(xAlertHandle, pvBuffer, ulBufferSize, "dfm_trace.psfs");

#ifdef DFM_SYMPTOM_CURRENT_TASK
	(void)xDfmAlertAddSymptom(xAlertHandle, DFM_SYMPTOM_CURRENT_TASK, ulDfmCalculateChecksum(pxData->acName, sizeof(pxData->acName)));
#endif

		/* Assumes "cloud port" is a UART or similar, that is always available. */
		if (xDfmAlertEnd(xAlertHandle) != DFM_SUCCESS)
		{
			DFM_DEBUG_PRINT("DFM: xDfmAlertEnd failed.\n");
		}

		(void)xTraceEnable(TRC_START);
}

#endif
