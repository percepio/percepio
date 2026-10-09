/*
 * Percepio DFM
 * Copyright 2023-2026 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * DFM serial port Cloud port
 */

#include <stddef.h>
#include <dfmCloudPort.h>
#include <dfmCloudPortConfig.h>
#include <dfm.h>
#include <dfmUtility.h>
#include <string.h>
#include <stdio.h>

#if (defined(DFM_CFG_ENABLED) && ((DFM_CFG_ENABLED) >= 1))

/* Prototype for the print function */
extern void vMainUARTPrintString( char * pcString );

static DfmCloudPortData_t *pxCloudPortData = (void*)0;

static uint16_t prvPrintDataAsHex(uint16_t seed, uint8_t* data, int size);
static DfmResult_t prvSerialPortUploadEntry(DfmEntryHandle_t xEntryHandle);

static uint16_t prvPrintDataAsHex(uint16_t seed, uint8_t* data, int size)
{
	uint16_t checksum = usDfmCalculateCrc16Ccitt(seed, data, (uint32_t)size);
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

		DFM_PRINT_ALERT_DATA(buf);

		if ( (i+1) % 20 == 0)
		{
			DFM_PRINT_ALERT_DATA((" ]]" LNBR));
			DFM_CFG_UNLOCK_SERIAL();
		}
	}

	if (i % 20 != 0)
	{
		DFM_PRINT_ALERT_DATA((" ]]" LNBR));
		DFM_CFG_UNLOCK_SERIAL();
	}

	return checksum;
}

static DfmResult_t prvSerialPortUploadEntry(DfmEntryHandle_t xEntryHandle)
{
	uint16_t checksum;
	uint32_t datalen;

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

	DFM_CFG_LOCK_SERIAL();
	DFM_PRINT_ALERT_DATA(LNBR "[[ DevAlert Data Begins ]]" LNBR);
	DFM_CFG_UNLOCK_SERIAL();

	checksum = prvPrintDataAsHex(0U, (uint8_t*)xEntryHandle, (int)datalen);

	snprintf(pxCloudPortData->buf, sizeof(pxCloudPortData->buf),
		"[[ DevAlert Data Ended. Checksum: %d ]]" LNBR,
		(unsigned int)checksum);

	DFM_CFG_LOCK_SERIAL();
	DFM_PRINT_ALERT_DATA(pxCloudPortData->buf);
	DFM_CFG_UNLOCK_SERIAL();
                
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
