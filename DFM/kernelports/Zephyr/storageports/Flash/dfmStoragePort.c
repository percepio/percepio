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
	void* pvBuffer;
	uint32_t ulBufferSize;
	uint32_t ulType;
	uint32_t ulOffset;
	uint16_t usExpectedEntryId;
	uint16_t usExpectedEntryCount;
} DfmWalkData_t;

static DfmStoragePortData_t* pxStoragePortData;

typedef struct DfmTraversionState {
	struct fcb_entry_ctx xEntryCtx;
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
static int prvVerifyMetadata(DfmStorageMetadata_t* pxFlashMetadata, DfmWalkData_t* pxWalkData)
{
	/* Verify content */
	if (pxFlashMetadata->ucStartMarker[0] != 0x44 ||
		pxFlashMetadata->ucStartMarker[1] != 0x46 ||
		pxFlashMetadata->ucStartMarker[2] != 0x6C ||
		pxFlashMetadata->ucStartMarker[3] != 0x61)
	{
		/* Incorrect entry, not much we can do about that */
		return 0;
	}

	if (pxFlashMetadata->ulType != pxWalkData->ulType)
	{
		if (pxWalkData->ulType == DFM_STORAGE_PORT_ALERT_TYPE)
		{
			/* We're looking for an alert, so we keep looking */
			return 0;
		}
		if (pxWalkData->ulType == DFM_STORAGE_PORT_PAYLOAD_TYPE)
		{
			/* We're looking for a payload but found something else, abort */
			//TODO: Investigate if we can have alerts which should have payloads which don't and makes us end up here
			return -1;
		}
	}

	if (pxFlashMetadata->usEntryId != pxWalkData->usExpectedEntryId)
	{
		return -1;
	}

	if (xDfmFlashMetadata.ulDataSize > (pxWalkData->ulBufferSize - pxWalkData->ulOffset))
	{
		/* Too big, not much we can do about that, abort */
		return -1;
	}

	return 0;
}

/**
 * A DFM specific implementation of fcb_walk. Since the fcb_walk-function provided by zephyr expect all data to be
 * traversed at once, while we need to traverse parts of it between function calls (i.e. over multiple
 * xDfmStorageGetAlert and xDfmStorageGetPayload calls), this stateful implementation is included with the storage port.
 * @param pxFcb Pointer to the struct which describes the FCB
 * @param pxTraversionState A container object for keeping the fcb context, needed between the traversions
 * @param pxWalkData The requested data (specifies, for example, whether a payload or alert is requested)
 * @return
 */
static int prvDfmFcbWalk(struct fcb *pxFcb, DfmTraversionState_t* pxTraversionState, DfmWalkData_t* pxWalkData)
{
	void* pvStartSector;
	if (pxTraversionState->xEntryCtx.loc.fe_sector == NULL)
		pvStartSector = (void*)pxFcb->f_oldest;
	else
		pvStartSector = (void*)pxTraversionState->xEntryCtx.loc.fe_sector;

	int lWalkResult = fcb_getnext(pxFcb, &pxTraversionState->xEntryCtx.loc);
	pxTraversionState->xEntryCtx.fap = pxFcb->fap;

	/* We've traversed all of the circular buffer, we're done*/
	if (lWalkResult == -ENOTSUP)
	{
		/*
		 * In case we've reached the end of the circular buffer, but are fetching payloads, we don't want to perform a
		 * rotation since we'll do another attempt at retrieving alerts (which is when the actual rotation should be made)
		 */
		if (pxWalkData->ulType == DFM_STORAGE_PORT_ALERT_TYPE)
			fcb_rotate(pxFcb);
		return -1;
	}

	/* Start by reading the metadata */
	int lReadResult = flash_area_read(
		pxTraversionState->xEntryCtx.fap,
		FCB_ENTRY_FA_DATA_OFF(pxTraversionState->xEntryCtx.loc),
		&xDfmFlashMetadata,
		sizeof(DfmStorageMetadata_t)
	);
	if (lReadResult != 0)
	{
		/* Read failed, not much we can do about that */
		return -1;
	}

	if (prvVerifyMetadata(&xDfmFlashMetadata, pxWalkData) != 0) {
		return -1;
	}

	if (pxWalkData->usExpectedEntryCount == 0)
	{
		/* This is the first entry found */
		pxWalkData->usExpectedEntryCount = xDfmFlashMetadata.usEntryCount;
	}

	/* We found the requested type */
	/* TODO: 64-bit support */
	lReadResult = flash_area_read(
		pxTraversionState->xEntryCtx.fap,
		FCB_ENTRY_FA_DATA_OFF(pxTraversionState->xEntryCtx.loc) + sizeof(DfmStorageMetadata_t),
		(void*)((uint32_t)pxWalkData->pvBuffer + pxWalkData->ulOffset),
		xDfmFlashMetadata.ulDataSize
	);
	if (lReadResult != 0)
	{
		/* Read failed, not much we can do about that, abort */
		return -1;
	}

	pxWalkData->usExpectedEntryId++;
	pxWalkData->ulOffset += xDfmFlashMetadata.ulDataSize;

	/* We've entered a new sector, increase the counter */
	if (pvStartSector != pxTraversionState->xEntryCtx.loc.fe_sector)
	{
		/* Since we've traversed the previous block, we can now rotate it */
		fcb_rotate(pxFcb);
	}

	if (pxWalkData->usExpectedEntryId != pxWalkData->usExpectedEntryCount)
	{
		/* We haven't found the entire payload */
		return 0;
	}

	return 1;
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
	DfmWalkData_t xWalkData = { 0 };
	int retVal = 0;

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

	xWalkData.ulType = DFM_STORAGE_PORT_ALERT_TYPE;
	xWalkData.pvBuffer = pvBuffer;
	xWalkData.ulBufferSize = ulBufferSize;
	xWalkData.usExpectedEntryId = 0;
	xWalkData.usExpectedEntryCount = 0;
	xWalkData.ulOffset = 0;

	/* Walk until it returns something else than 0. Greater than 0 is success, less than 0 is fail */
	do
	{
		retVal = prvDfmFcbWalk(&xFlashCircularBuffer, &xTraversionState, &xWalkData);
	} while (retVal == 0);

	/* Reset the State, we're done for now. This happens either due to an error or due to that we've reached the end of the buffer */
	if (retVal == -1)
	{
		memset(&xTraversionState, 0, sizeof(struct fcb_entry_ctx));
	}

	return retVal > 0 ? DFM_SUCCESS : DFM_FAIL;
}

DfmResult_t xDfmStoragePortStorePayloadChunk(DfmEntryHandle_t xEntryHandle, uint32_t ulOverwrite)
{
	return prvDfmStoragePortWrite(xEntryHandle, DFM_STORAGE_PORT_PAYLOAD_TYPE, ulOverwrite);
}

DfmResult_t xDfmStoragePortGetPayloadChunk(char* szSessionId, uint32_t ulAlertId, void* pvBuffer, uint32_t ulBufferSize)
{
	DfmWalkData_t xWalkData = { 0 };
	int retVal = 0;

	/* We don't need these to find a payload */
	(void)szSessionId;
	(void)ulAlertId;

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

	xWalkData.ulType = DFM_STORAGE_PORT_PAYLOAD_TYPE;
	xWalkData.pvBuffer = pvBuffer;
	xWalkData.ulBufferSize = ulBufferSize;
	xWalkData.usExpectedEntryId = 0;
	xWalkData.usExpectedEntryCount = 0;
	xWalkData.ulOffset = 0;

	/* Walk until it returns something else than 0. Greater than 0 is success, less than 0 is fail */
	do
	{
		retVal = prvDfmFcbWalk(&xFlashCircularBuffer, &xTraversionState, &xWalkData);
	} while (retVal == 0);

	return retVal > 0 ? DFM_SUCCESS : DFM_FAIL;
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
