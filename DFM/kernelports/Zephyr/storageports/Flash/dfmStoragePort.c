/*
 * Percepio DFM v2.0.0
 * Copyright 2023 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr Flash Storage port
 */

#include "dfmStoragePort.h"
#include <string.h>

/**
 * @todo
 * 	- Session ID
 * 		- User could set this through an API call
 * 	- Device Identifier (Not used in FreeRTOS, from connection to cloud)
 * 		- Check with helge, do they have any ID in settings/memory
 * 		- Their tool could handle it.
 */

#if (defined(DFM_CFG_ENABLED) && ((DFM_CFG_ENABLED) >= 1))

#include <dfm.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/fs/fcb.h>
#include <zephyr/storage/flash_map.h>

 /**
  * @internal DFM Flash entry structure
  */
typedef struct DfmStorageMetadata {
	uint8_t ucStartMarker[4];								/**< Start marker buffer */
	uint16_t usEntryId;
	uint16_t usEntryCount;
	uint32_t ulType;
	uint32_t ulDataSize;
} __attribute__((aligned(8))) DfmStorageMetadata_t;

 /**
  * @def DFM_STORAGE_PORT_FCB_AREA_NAME
  * @brief Specifies the area/partition name for the DFM port.
  *
  * This value is fixed since it depends on the boards dts file. While it might
  * be possible to get a token label from KConfig, Zephyr prefers not mixing
  * KConfig and device tree configurations. This seems like the most suitable
  * solution for now.
  */
#define DFM_STORAGE_PORT_FCB_AREA_NAME dfm_partition

/* Ensure that the sector count and size is compatible with the given partition */
#if (FIXED_PARTITION_SIZE(dfm_partition) < (CONFIG_PERCEPIO_DFM_CFG_STORAGE_PORT_FCB_SECTOR_COUNT * CONFIG_PERCEPIO_DFM_CFG_STORAGE_PORT_FCB_SECTOR_SIZE))
#error "DevAlert: You have specified a sector count and size combination which is larger than your dfm_storage partition!"
#endif

#define DFM_STORAGE_PORT_FCB_SECTOR_SIZE_ALIGNED (((CONFIG_PERCEPIO_DFM_CFG_STORAGE_PORT_FCB_SECTOR_SIZE) / 8U) * 8U)

#define DFM_STORAGE_PORT_ALERT_TYPE		0x34561842
#define DFM_STORAGE_PORT_PAYLOAD_TYPE	0x82713124

DfmResult_t prvDfmStoragePortWrite(DfmEntryHandle_t xEntryHandle, uint32_t ulType, uint32_t ulOverwrite);

/* Stores active DFM flash entry */
static DfmStorageMetadata_t xDfmFlashMetadata = { 0 };

/* Stores active FCB entry structure */
struct fcb_entry xFcbEntry = {0};

/* Stores active FCB structure */
struct fcb xFlashCircularBuffer = {0};

/* Stores FCB sector mappings */
struct flash_sector xFlashCircularBufferSector[CONFIG_PERCEPIO_DFM_CFG_STORAGE_PORT_FCB_SECTOR_COUNT];

typedef struct DfmWalkData
{
	uint32_t ulCurrentType;
	uint32_t ulOffset;
	uint16_t usExpectedEntryId;
	uint16_t usExpectedEntryCount;
} DfmWalkData_t;

static DfmStoragePortData_t* pxStoragePortData;

typedef struct DfmTraversionState {
	struct fcb_entry_ctx xEntryCtx;
	uint8_t ucIsDirty;
} DfmTraversionState_t;

static DfmTraversionState_t xTraversionState = {
	.xEntryCtx = {}
};

/**
 * Verify the metadata header, making sure that the data being held in the current entry matches what was expected
 * @param pxFlashMetadata The static DfmStorageMetaData_t struct at the top of this file, contains the metadata just
 * 					 read from flash.
 * @param pxWalkData The entry request specification, essentially. What are we expecting
 * @return
 */
static int prvVerifyMetadata(DfmStorageMetadata_t* pxFlashMetadata, DfmWalkData_t* pxWalkData, uint32_t ulBufferSize)
{
	/* Verify content */
	if (pxFlashMetadata->ucStartMarker[0] != 0x44 ||
		pxFlashMetadata->ucStartMarker[1] != 0x46 ||
		pxFlashMetadata->ucStartMarker[2] != 0x6C ||
		pxFlashMetadata->ucStartMarker[3] != 0x61)
	{
		/* Incorrect entry, not much we can do about that */
		return -1;
	}

	if (pxFlashMetadata->ulType != pxWalkData->ulCurrentType)
	{
		return -1;
	}

	if (pxFlashMetadata->usEntryId != pxWalkData->usExpectedEntryId)
	{
		return -1;
	}

	if (xDfmFlashMetadata.ulDataSize > (ulBufferSize - pxWalkData->ulOffset))
	{
		/* Too big, not much we can do about that, abort */
		return -1;
	}

	return 0;
}

/**
 * Get the next payload chunk/alert from the fcb. This can mean reading multiple fcb entries.
 * @param pxFcb Pointer to the struct which describes the FCB
 * @param pxTraversionState A container object for keeping the fcb context, needed between the traversions
 * @param pvBuffer The buffer containing the current alert/payload chunk which is supposed to be retrieved
 * @param ulBufferSize Size of the aforementioned buffer
 * @return 0 => success, !0 => fail
 */
static int prvDfmGetNext(struct fcb *pxFcb, DfmTraversionState_t* pxTraversionState, void* pvBuffer, uint32_t ulBufferSize)
{
	DfmWalkData_t xWalkData = { 0 };
	void* pvStartSector = NULL;
	int lWalkResult = 0;
	int lReadResult = 0;

	if (pxTraversionState->xEntryCtx.loc.fe_sector == NULL)
		pvStartSector = (void*)pxFcb->f_oldest;
	else
		pvStartSector = (void*)pxTraversionState->xEntryCtx.loc.fe_sector;

	while (1)
	{
		lWalkResult = fcb_getnext(pxFcb, &pxTraversionState->xEntryCtx.loc);
		pxTraversionState->xEntryCtx.fap = pxFcb->fap;

		if (lWalkResult == -ENOTSUP)
		{
			if (pxTraversionState->ucIsDirty)
			{
				fcb_rotate(pxFcb);
			}

			/* Reset the traversion context, we've traversed the FCB */
			memset(&xTraversionState, 0, sizeof(DfmTraversionState_t));

			return -1;
		}

		pxTraversionState->ucIsDirty = 1;

		if (pvStartSector != pxTraversionState->xEntryCtx.loc.fe_sector)
		{
			/* We've entered a new sector, increase the counter */
			/* Since we've traversed the previous block, we can now rotate it */
			fcb_rotate(pxFcb);
		}

		/* Start by reading the metadata */
		lReadResult = flash_area_read(
			pxTraversionState->xEntryCtx.fap,
			FCB_ENTRY_FA_DATA_OFF(pxTraversionState->xEntryCtx.loc),
			&xDfmFlashMetadata,
			sizeof(DfmStorageMetadata_t)
		);

		if (lReadResult != 0)
		{
			/* Read failed, reset the buffer and continue */
			memset(pvBuffer, 0, ulBufferSize);
			memset(&xWalkData, 0, sizeof(DfmWalkData_t));
			continue;
		}

		if (prvVerifyMetadata(&xDfmFlashMetadata, &xWalkData, ulBufferSize) != 0)
		{
			/* Start over, this will always happen at the first read, only on failure for subsequent reads */
			if (xDfmFlashMetadata.usEntryId == 0) {
				xWalkData.ulCurrentType = xDfmFlashMetadata.ulType;
				xWalkData.usExpectedEntryId = 0;
				xWalkData.usExpectedEntryCount = xDfmFlashMetadata.usEntryCount;
				xWalkData.ulOffset = 0;
			}
			else
			{
				memset(&xWalkData, 0, sizeof(DfmWalkData_t));
				continue;
			}
		}

		/* Read the actual data */
		/* TODO: 64bit compatiblity */
		lReadResult = flash_area_read(
			pxTraversionState->xEntryCtx.fap,
			FCB_ENTRY_FA_DATA_OFF(pxTraversionState->xEntryCtx.loc) + sizeof(DfmStorageMetadata_t),
			(void*)((uint32_t)pvBuffer + xWalkData.ulOffset),
			xDfmFlashMetadata.ulDataSize
		);

		if (lReadResult != 0)
		{
			/* Read failed, reset the buffer and continue */
			memset(pvBuffer, 0, ulBufferSize);
			memset(&xWalkData, 0, sizeof(DfmWalkData_t));
			continue;
		}

		xWalkData.usExpectedEntryId++;
		xWalkData.ulOffset += xDfmFlashMetadata.ulDataSize;

		if (xWalkData.usExpectedEntryId == xWalkData.usExpectedEntryCount)
		{
			/* Found the entire alert/payload chunk */
			return 0;
		}
	}
}

/* This function is used to avoid "unreachable code" warnings */
int prvDfmReturn(int val)
{
	return val;
}

DfmResult_t xDfmStoragePortInitialize(DfmStoragePortData_t *pxBuffer)
{
	if (pxBuffer == (void*)0)
	{
		return DFM_FAIL;
	}

	if ((CONFIG_PERCEPIO_DFM_CFG_STORAGE_PORT_FCB_SECTOR_SIZE) <= prvDfmReturn(sizeof(xDfmFlashMetadata) + sizeof(struct fcb_entry)))
	{
		/* No room for any data */
		return DFM_FAIL;
	}

	/* Configure FCB sectors */
	for (int32_t i = 0; i < ARRAY_SIZE(xFlashCircularBufferSector); i++) {
		xFlashCircularBufferSector[i].fs_off = i * CONFIG_PERCEPIO_DFM_CFG_STORAGE_PORT_FCB_SECTOR_SIZE;
		xFlashCircularBufferSector[i].fs_size = CONFIG_PERCEPIO_DFM_CFG_STORAGE_PORT_FCB_SECTOR_SIZE;
	}

	/* Configure FCB section */
	xFlashCircularBuffer.f_sector_cnt = ARRAY_SIZE(xFlashCircularBufferSector);
	xFlashCircularBuffer.f_sectors = xFlashCircularBufferSector;

	/* Initialize FCB */
	if (fcb_init(FIXED_PARTITION_ID(DFM_STORAGE_PORT_FCB_AREA_NAME), &xFlashCircularBuffer) != 0)
	{
		return DFM_FAIL;
	}

	pxStoragePortData = pxBuffer;
	pxStoragePortData->ulInitialized = 1;

	return 0;
}

DfmResult_t xDfmStoragePortStoreSession(void* pvData, uint32_t ulSize)
{
	if (pxStoragePortData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxStoragePortData->ulInitialized == 0)
	{
		return DFM_FAIL;
	}

	/* Check if parameters are valid. */
	if(pvData != (void*)0)
	{
		return DFM_FAIL;
	}

	if (ulSize == 0)
	{
		return DFM_FAIL;
	}

	/* TODO: Store session outside FCB (or in another FCB with just one small entry) */

	return DFM_FAIL;
}

DfmResult_t xDfmStoragePortGetSession(void* pvBuffer, uint32_t ulBufferSize)
{
	if (pxStoragePortData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxStoragePortData->ulInitialized == 0)
	{
		return DFM_FAIL;
	}

	return DFM_FAIL;
}


DfmResult_t xDfmStoragePortStoreAlert(DfmEntryHandle_t xEntryHandle, uint32_t ulOverwrite)
{
	return prvDfmStoragePortWrite(xEntryHandle, DFM_STORAGE_PORT_ALERT_TYPE, ulOverwrite);
}

DfmResult_t xDfmStoragePortGetAlert(void* pvBuffer, uint32_t ulBufferSize)
{
	DfmEntryHandle_t xEntryHandle = 0;

	if (pxStoragePortData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxStoragePortData->ulInitialized == 0)
	{
		return DFM_FAIL;
	}

	if (pvBuffer == (void*)0)
	{
		return DFM_FAIL;
	}

	if (ulBufferSize == 0)
	{
		return DFM_FAIL;
	}

	while (xDfmEntryCreateAlertFromBuffer(&xEntryHandle) == DFM_FAIL)
	{
		if (prvDfmGetNext(&xFlashCircularBuffer, &xTraversionState, pvBuffer, ulBufferSize) != 0)
		{
			return DFM_FAIL;
		}
	}

	return DFM_SUCCESS;
}

DfmResult_t xDfmStoragePortStorePayloadChunk(DfmEntryHandle_t xEntryHandle, uint32_t ulOverwrite)
{
	return prvDfmStoragePortWrite(xEntryHandle, DFM_STORAGE_PORT_PAYLOAD_TYPE, ulOverwrite);
}

DfmResult_t xDfmStoragePortGetPayloadChunk(char* szSessionId, uint32_t ulAlertId, void* pvBuffer, uint32_t ulBufferSize)
{
	/* We don't need these to find a payload */
	(void)szSessionId;
	(void)ulAlertId;
	DfmEntryHandle_t xEntryHandle = 0;

	if (pxStoragePortData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxStoragePortData->ulInitialized == 0)
	{
		return DFM_FAIL;
	}

	if (pvBuffer == (void*)0)
	{
		return DFM_FAIL;
	}

	if (ulBufferSize == 0)
	{
		return DFM_FAIL;
	}

	if (prvDfmGetNext(&xFlashCircularBuffer, &xTraversionState, pvBuffer, ulBufferSize) != 0)
	{
		return DFM_FAIL;
	}

	if (xDfmEntryCreatePayloadChunkFromBuffer(szSessionId, ulAlertId, &xEntryHandle) == DFM_FAIL)
	{
		return DFM_FAIL;
	}

	return DFM_SUCCESS;
}

DfmResult_t prvDfmStoragePortWrite(DfmEntryHandle_t xEntryHandle, uint32_t ulType, uint32_t ulOverwrite)
{
	uint32_t ulBytesWritten = 0;
	uint32_t ulBytesToWrite;
	void* pvData = (void*)xEntryHandle;
	uint32_t ulRemainingBytes = 0;
	const uint32_t ulMaxUsableSectorSize = (DFM_STORAGE_PORT_FCB_SECTOR_SIZE_ALIGNED) - sizeof(xDfmFlashMetadata) - sizeof(struct fcb_entry);

	if (pxStoragePortData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxStoragePortData->ulInitialized == 0)
	{
		return DFM_FAIL;
	}

	if (xEntryHandle == 0)
	{
		return DFM_FAIL;
	}

	if (xDfmEntryGetSize(xEntryHandle, &ulRemainingBytes) == DFM_FAIL)
	{
		return DFM_FAIL;
	}

	if (ulRemainingBytes == 0)
	{
		return DFM_FAIL;
	}

	if ((ulType != DFM_STORAGE_PORT_ALERT_TYPE) && (ulType != DFM_STORAGE_PORT_PAYLOAD_TYPE))
	{
		return DFM_FAIL;
	}

	/* Configure start markers, used to find the start of flash sections
	 * when parsing binary blobs. */
	xDfmFlashMetadata.ucStartMarker[0] = 0x44;	/* 'D' */
	xDfmFlashMetadata.ucStartMarker[1] = 0x46;	/* 'F' */
	xDfmFlashMetadata.ucStartMarker[2] = 0x6C;	/* 'l' */
	xDfmFlashMetadata.ucStartMarker[3] = 0x61;	/* 'a' */

	xDfmFlashMetadata.ulType = ulType;

	/* Calculate the number of segments we have to write */
	xDfmFlashMetadata.usEntryCount = (uint16_t)((ulRemainingBytes - 1U) / ulMaxUsableSectorSize) + 1U;

	/* Reset segment id to null position (the first segment starts at 1, setting this to 0 here
	 * is a convenient method of allowing us to increment this later on.) */
	xDfmFlashMetadata.usEntryId = 0;

	while (ulRemainingBytes > 0)
	{
		/* Calculate write size in regard to the maximum write size for the FCB API. */
		/* TODO: Check calculation */
		ulBytesToWrite = ulRemainingBytes + sizeof(xDfmFlashMetadata);
		if (ulBytesToWrite > ulMaxUsableSectorSize)
		{
			ulBytesToWrite = ulMaxUsableSectorSize;
		}
		else
		{
			/* Align the number of bytes to be written to 8 due to that the flash wants 8-byte aligned writes */
			ulBytesToWrite = (((ulBytesToWrite + 7U) / 8U) * 8U);
			ulRemainingBytes = ulBytesToWrite - sizeof(xDfmFlashMetadata);
		}

		/* Reset entry data */
		memset(&xFcbEntry, 0, sizeof(xFcbEntry));

		/* Get free fcb sector, if none available return. */
		if (fcb_append(&xFlashCircularBuffer, (uint16_t)ulBytesToWrite, &xFcbEntry) != 0)
		{
			if (ulOverwrite == 1)
			{
				if (fcb_rotate(&xFlashCircularBuffer) != 0)
				{
					return DFM_FAIL;
				}

				if (fcb_append(&xFlashCircularBuffer, (uint16_t)ulBytesToWrite, &xFcbEntry) != 0)
				{
					return DFM_FAIL;
				}
			}
			else
			{
				return DFM_FAIL;
			}
		}

		/* Store size of entry data in this sector */
		xDfmFlashMetadata.ulDataSize = ulBytesToWrite - sizeof(DfmStorageMetadata_t);

		/* Write metadata */
		if (flash_area_write(xFlashCircularBuffer.fap, FCB_ENTRY_FA_DATA_OFF(xFcbEntry), &xDfmFlashMetadata, sizeof(DfmStorageMetadata_t)) != 0)
		{
			return DFM_FAIL;
		}

		/* Write entry data */
		/* TODO: 64-bit compatible */
		volatile int return_write_value = flash_area_write(xFlashCircularBuffer.fap, FCB_ENTRY_FA_DATA_OFF(xFcbEntry) + sizeof(DfmStorageMetadata_t), (void*)((uint32_t)pvData + ulBytesWritten), xDfmFlashMetadata.ulDataSize);
		if (return_write_value != 0)
		{
			return DFM_FAIL;
		}

		/* Finalize write with CRC */
		if (fcb_append_finish(&xFlashCircularBuffer, &xFcbEntry) != 0)
		{
			return DFM_FAIL;
		}

		/* Update segment id */
		xDfmFlashMetadata.usEntryId++;

		/* Update byte counters */
		ulRemainingBytes -= xDfmFlashMetadata.ulDataSize;
		ulBytesWritten += xDfmFlashMetadata.ulDataSize;
    }

	return DFM_SUCCESS;
}

#endif
