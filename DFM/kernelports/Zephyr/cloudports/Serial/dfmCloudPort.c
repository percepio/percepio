/*
 * Percepio DFM v2.0.0
 * Copyright 2023 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "dfmCloudPort.h"
#include <dfmCloudPortConfig.h>
#include <dfm.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/crc.h>

extern bool globFailRun;
static DfmCloudPortData_t* pxCloudPortData = (void*)0;

static uint16_t prvPrintDataAsHex(uint16_t seed, uint8_t* data, uint32_t size)
{
	uint16_t crc = crc16_ccitt(seed, data, size);
	int i;
	char buf[10];

	for (i = 0; i < size; i++)
	{
		uint8_t byte = data[i];
		snprintf(buf, sizeof(buf), " %02X", (unsigned int)byte);

		if (i % 20 == 0)
		{
			DFM_CFG_LOCK_SERIAL();
			DFM_PRINT_ALERT_DATA(("[[ DATA:"));
		}

		DFM_PRINT_ALERT_DATA("%s", buf);

		if ( (i+1) % 20 == 0)
		{
			DFM_PRINT_ALERT_DATA((" ]]\n"));
			DFM_CFG_UNLOCK_SERIAL();
		}
	}

	if (i % 20 != 0)
	{
		DFM_PRINT_ALERT_DATA((" ]]\n"));
		DFM_CFG_UNLOCK_SERIAL();
	}

	return crc;
}

static DfmResult_t prvSerialPortUploadEntry(DfmEntryHandle_t xEntryHandle)
{
	//TODO: We will want something similar to the TraceUnsignedBaseType here, DfmUnsignedBaseType?
	uint32_t checksum;
	uint32_t datalen;

	if (pxCloudPortData == (void*)0)
		return DFM_FAIL;

	if (xEntryHandle == 0)
		return DFM_FAIL;

	if (xDfmEntryGetSize(xEntryHandle, &datalen) == DFM_FAIL)
		return DFM_FAIL;

	if (datalen > 0xFFFF)
		return DFM_FAIL;

	/*
	 * TODO: Investigate further if zephyr has some way of locking the serial while sending the data so that
	 * 		 the complete content of the data block can be sent in one go.
	 */
	printk("\n[[ DevAlert Data Begins ]]\n");
	checksum = prvPrintDataAsHex(0, (uint8_t*)xEntryHandle, datalen);
	printk("\n[[ DevAlert Data Ended. Checksum: %d ]]\n", checksum);

	return DFM_SUCCESS;
}

DfmResult_t xDfmCloudPortInitialize(DfmCloudPortData_t* pxBuffer)
{
	pxCloudPortData = pxBuffer;
	return DFM_SUCCESS;
}

DfmResult_t xDfmCloudPortSendAlert(DfmEntryHandle_t xEntryHandle)
{
	//Simulate failing chunk sends to use the storage
	if (globFailRun) {
		printk("Ooops, couldn't send over serial, I better store it in buffer\n");
		return DFM_FAIL;
	}
	return prvSerialPortUploadEntry(xEntryHandle);
}

DfmResult_t xDfmCloudPortSendPayloadChunk(DfmEntryHandle_t xEntryHandle)
{
	return prvSerialPortUploadEntry(xEntryHandle);
}
