/*
 * Percepio DFM v2.0.0
 * Copyright 2023 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * FLASH Example implementation
 */

#include <dfm.h>

#if (defined(DFM_CFG_ENABLED) && ((DFM_CFG_ENABLED) >= 1))

#include "dfmStoragePort.h"

#include <string.h> // for memcpy

/* Include your specific flash header file */
#include "flash.h"
#include "stm32l4xx_hal.h"

#define DFM_STORAGE_PORT_ALERT_TYPE		0x34561842
#define DFM_STORAGE_PORT_PAYLOAD_TYPE	0x82713124

typedef struct{
	uint8_t data[DFM_CFG_FLASHSTORAGE_SIZE];
    uint32_t alert_storage_counter;
} dfmFlashData_t;

dfmFlashData_t dfmFlashData __attribute__( ( section( ".dfm_alert" ), aligned (8) ) ) = { {0}, 0 };

uint32_t ulWrOffset = 0;
uint32_t ulRdOffset = 0;

static DfmResult_t prvDfmStoragePortWrite(DfmEntryHandle_t xEntryHandle, uint32_t ulType, uint32_t ulOverwrite);

 /**
  * @internal DFM Flash entry structure
  */
typedef struct DfmStorageMetadata {
	uint32_t dummy;
} DfmStorageMetadata_t;

DfmResult_t xDfmStoragePortInitialize(DfmStoragePortData_t *pxBuffer)
{
	ulWrOffset = 0;
	ulRdOffset = 0;
	return DFM_SUCCESS;
}

DfmResult_t xDfmStoragePortStoreSession(void* pvData, uint32_t ulSize)
{
	return DFM_FAIL;
}

DfmResult_t xDfmStoragePortGetSession(void* pvBuffer, uint32_t ulBufferSize)
{
	return DFM_FAIL;
}


DfmResult_t xDfmStoragePortStoreAlert(DfmEntryHandle_t xEntryHandle, uint32_t ulOverwrite)
{
	return prvDfmStoragePortWrite(xEntryHandle, DFM_STORAGE_PORT_ALERT_TYPE, ulOverwrite);
}

DfmResult_t xDfmStoragePortGetAlert(void* pvBuffer, uint32_t ulBufferSize)
{
	if (*((uint32_t*)&dfmFlashData.data[ulRdOffset]) == 0xffffffff)
	{
		// Not written
		return DFM_FAIL;
	}
	uint32_t ulSize;
	if (xDfmEntryGetSize((DfmEntryHandle_t)&dfmFlashData.data[ulRdOffset], &ulSize) == DFM_FAIL)
	{
		return DFM_FAIL;
	}
	// Read data into buffer
	if (ulSize > ulBufferSize)
	{
		ulSize = ulBufferSize;
	}
	memcpy(pvBuffer, &dfmFlashData.data[ulRdOffset], ulSize);
	ulRdOffset += ulSize;
	ulRdOffset += (8 - (ulSize % 8));
	return DFM_SUCCESS;
}

DfmResult_t xDfmStoragePortStorePayloadChunk(DfmEntryHandle_t xEntryHandle, uint32_t ulOverwrite)
{
	return prvDfmStoragePortWrite(xEntryHandle, DFM_STORAGE_PORT_PAYLOAD_TYPE, ulOverwrite);
}

DfmResult_t xDfmStoragePortGetPayloadChunk(char* szSessionId, uint32_t ulAlertId, void* pvBuffer, uint32_t ulBufferSize)
{
	return xDfmStoragePortGetAlert(pvBuffer, ulBufferSize);
}

/* Note: This function assumes the affected flash pages are only used for DFM alert storage.
 * It does not preserve other data that might be present on these pages (future improvement!) */
static DfmResult_t prvDfmStoragePortWrite(DfmEntryHandle_t xEntryHandle, uint32_t ulType, uint32_t ulOverwrite)
{
	DFM_DEBUG_PRINT("prvDfmStoragePortWrite\n");

	if (xEntryHandle == 0)
	{
		return DFM_FAIL;
	}
	uint32_t ulSize;
	if (xDfmEntryGetSize(xEntryHandle, &ulSize) == DFM_FAIL)
	{
		return DFM_FAIL;
	}

	if ((ulType != DFM_STORAGE_PORT_ALERT_TYPE) && (ulType != DFM_STORAGE_PORT_PAYLOAD_TYPE))
	{
		return DFM_FAIL;
	}

	uint32_t ulDst = (uint32_t)&dfmFlashData.data[ulWrOffset];

	/* Write to destination */
	if (ulWrOffset == 0)
	{

		/* This erases the affected flash pages, needed before write. */
		if (xDfmStoragePortReset() != DFM_SUCCESS)
		{
			return DFM_FAIL;
		}
	}
	
	ulWrOffset += ulSize;
	ulWrOffset += (8 - (ulSize % 8));

	if ( ulWrOffset >= sizeof(dfmFlashData.data))
	{
		/* If this happens, you may increase DFM_DEMO_FLASHSTORAGE_SIZE */
		DFM_ERROR_PRINT("\nDFM: Error - Not enough space in dfmFlashData.data.\n");
		return DFM_FAIL;
	}

	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);

	if (FLASH_write_at(ulDst, xEntryHandle, ulSize) != 0)
	{
		return DFM_FAIL;
	}

	return DFM_SUCCESS;
}

DfmResult_t xDfmStoragePortReset(void)
{
	DFM_DEBUG_PRINT("xDfmStoragePortReset()\n");

	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);

	if (FLASH_unlock_erase((uint32_t)&dfmFlashData, sizeof(dfmFlashData)) != 0)
	{
		return DFM_FAIL;
	}

	return DFM_SUCCESS;
}

#endif
