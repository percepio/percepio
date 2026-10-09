/*
 * Percepio DFM
 * Copyright 2023-2026 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * DFM cloud port using ARM ITM.
 */

#include <stddef.h>
#include <dfmCloudPort.h>
#include <dfmCloudPortConfig.h>
#include <dfm.h>
#include <string.h>
#include <stdio.h>

#if (defined(DFM_CFG_ENABLED) && ((DFM_CFG_ENABLED) >= 1))
	
static DfmCloudPortData_t *pxCloudPortData = (void*)0;

static DfmResult_t prvSerialPortUploadEntry(DfmEntryHandle_t xEntryHandle);

/* This blocks until there is room in the ITM FIFO (TPIU) */
#define itm_write_32(__port, __data) \
{\
		while (ITM->PORT[__port].u32 == 0) { /* Do nothing */ } \
		ITM->PORT[__port].u32 = __data;    /* Write the data */ \
}

/* This blocks until there is room in the ITM FIFO (TPIU) */
#define itm_write_8(__port, __data) \
{\
		while (ITM->PORT[__port].u8 == 0) { /* Do nothing */ } \
		ITM->PORT[__port].u8 = __data;    /* Write the data */ \
}


void prvItmWrite(void* ptrData, uint32_t size, int32_t* ptrBytesWritten)
{
	uint32_t* ptr32 = (uint32_t*)ptrData;

	TRC_ASSERT(size % 4 == 0);
	TRC_ASSERT(ptrBytesWritten != 0);

	*ptrBytesWritten = 0;
	
	if ((CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk) &&    /* Trace enabled? */ \
		(ITM->TCR & ITM_TCR_ITMENA_Msk) &&                /* ITM enabled? */ \
		(ITM->TER & (1UL << (DFM_CFG_ITM_PORT))))                 /* ITM port enabled? */
	{
		while (*ptrBytesWritten < (int32_t)size)
		{
			itm_write_32(DFM_CFG_ITM_PORT, *ptr32);
			ptr32++;
			*ptrBytesWritten += 4;
		}
	}
}

 
/* When using ITM logging over STLINK v2 together with IAR Embedded Workbench,
   the last ~2 KB of data is sometimes not emitted to the log file until more
   data is written. It seems there is some buffer that isn't always flushed.

   As a workaround, use vDfmCloudPortFlushWithDummyData to emit 2 KB of dummy
   data after each alert to ensure all DFM data is written to the ITM log file. 
   This extra data in between the DFM alerts is ignored by the bin2alerts script
   so is not ingested by Percepio Detect.

   This issues seems to be specific for STLINK probes. IAR i-jet probes does not
   seem to have this issue and has worked fine without the flushing in our tests.

   To use this, add the following in your dfmConfig.h:

   extern void vDfmCloudPortFlushWithDummyData(void);
   #define DFM_CFG_AFTER_ALERT_SEND(pxAlert) vDfmCloudPortFlushWithDummyData();
*/
void vDfmCloudPortFlushWithDummyData(void)
{
	for (int i=0; i < 512; i++)
	{
		itm_write_32(2, 0xA5A5A5A5);
	}
}

static DfmResult_t prvSerialPortUploadEntry(DfmEntryHandle_t xEntryHandle)
{
	uint32_t datalen;
	int32_t bytesWritten;

	if (pxCloudPortData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (xEntryHandle == 0)
	{
		return DFM_FAIL;
	}

	if (xDfmEntryGetSize(xEntryHandle, &datalen) == DFM_FAIL)
	{
		return DFM_FAIL;
	}

	if (datalen > 0xFFFF)
	{
		return DFM_FAIL;
	}

	// Write the DFM data in raw binary format to the ITM port.
	prvItmWrite((uint8_t*)xEntryHandle, datalen, &bytesWritten);

	if (bytesWritten != datalen)
	{
		return DFM_FAIL;
	}

	return DFM_SUCCESS;
}

DfmResult_t xDfmCloudPortInitialize(DfmCloudPortData_t* pxBuffer)
{
	pxCloudPortData = pxBuffer;
	return DFM_SUCCESS;
}

DfmResult_t xDfmCloudPortSendAlert(DfmEntryHandle_t xEntryHandle)
{
	return prvSerialPortUploadEntry(xEntryHandle);
}

DfmResult_t xDfmCloudPortSendPayloadChunk(DfmEntryHandle_t xEntryHandle)
{
	return prvSerialPortUploadEntry(xEntryHandle);
}

#endif
