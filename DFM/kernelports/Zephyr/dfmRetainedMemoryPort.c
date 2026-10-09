/*
 * Percepio DFM
 * Copyright 2023-2026 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * DFM Retained Memory Port for Zephyr
 */

#include <dfm.h>

#if (defined(DFM_CFG_ENABLED) && (DFM_CFG_ENABLED >= 1)) && (defined(DFM_CFG_RETAINED_MEMORY) && (DFM_CFG_RETAINED_MEMORY >= 1))
	
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/retained_mem.h>
#include <zephyr/retention/retention.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#define DFM_RETENTION_NODE DT_NODELABEL(retention0)
#define DFM_RETAINED_MEMORY_NODE DT_PARENT(DFM_RETENTION_NODE)
#define DFM_RETENTION_AREA_OFFSET DT_PROP_BY_IDX(DFM_RETENTION_NODE, reg, 0)
#define DFM_RETENTION_AREA_SIZE DT_REG_SIZE(DFM_RETENTION_NODE)
#define DFM_RETENTION_PREFIX_SIZE DT_PROP_LEN(DFM_RETENTION_NODE, prefix)
#define DFM_RETENTION_FIRST_PREFIX_BYTE DT_PROP_BY_IDX(DFM_RETENTION_NODE, prefix, 0)
#define DFM_RETENTION_SUM_SIZE sizeof(uint32_t)
#define DFM_RETENTION_DATA_SIZE \
	(DFM_RETENTION_AREA_SIZE - DFM_RETENTION_PREFIX_SIZE - DFM_RETENTION_SUM_SIZE)

/* retained_mem_clear() clears the complete parent device. These assertions
 * ensure that retention0 owns that device and that zeroing it invalidates the
 * configured prefix without touching any unrelated retained data.
 */
BUILD_ASSERT(DFM_RETENTION_AREA_OFFSET == 0U,
	"DFM retention0 must start at offset zero");
BUILD_ASSERT(DFM_RETENTION_AREA_SIZE == DT_REG_SIZE(DT_GPARENT(DFM_RETENTION_NODE)),
	"DFM retention0 must own its retained-memory device");
BUILD_ASSERT(DFM_RETENTION_FIRST_PREFIX_BYTE != 0U,
	"DFM retention prefix must not begin with zero");
BUILD_ASSERT(DT_PROP(DFM_RETENTION_NODE, checksum) == 0U,
	"DFM uses its own incremental SUM32 checksum");
BUILD_ASSERT(DFM_RETENTION_AREA_SIZE >
	(DFM_RETENTION_PREFIX_SIZE + DFM_RETENTION_SUM_SIZE),
	"DFM retention area is too small");

static const struct device *pxRetention0 = DEVICE_DT_GET(DFM_RETENTION_NODE);
static const struct device *pxRetainedMemory = DEVICE_DT_GET(DFM_RETAINED_MEMORY_NODE);
static uint32_t ulRetainedMemorySum;

/* Add little-endian 32-bit words, with zero padding at the end. Using the
 * absolute offset makes the result independent of DFM's write chunk sizes.
 */
static uint32_t prvSum32Update(uint32_t ulSum, const uint8_t* pucData,
	uint32_t ulSize, uint32_t ulOffset)
{
	while ((ulSize > 0U) && ((ulOffset & 3U) != 0U))
	{
		ulSum += (uint32_t)*pucData << ((ulOffset & 3U) * 8U);
		pucData++;
		ulSize--;
		ulOffset++;
	}

	while (ulSize >= sizeof(uint32_t))
	{
		ulSum += sys_get_le32(pucData);
		pucData += sizeof(uint32_t);
		ulSize -= sizeof(uint32_t);
		ulOffset += sizeof(uint32_t);
	}

	while (ulSize > 0U)
	{
		ulSum += (uint32_t)*pucData << ((ulOffset & 3U) * 8U);
		pucData++;
		ulSize--;
		ulOffset++;
	}

	return ulSum;
}

DfmResult_t xDfmRetainedMemoryPortInitialize(DfmRetainedMemoryPortData_t* pxBuffer)
{
	(void)pxBuffer;

	if ((!device_is_ready(pxRetention0)) || (!device_is_ready(pxRetainedMemory)))
	{
		return DFM_FAIL;
	}

	return DFM_SUCCESS;
}

DfmResult_t xDfmRetainedMemoryPortClear(void)
{
	/* This is one backend clear instead of retention_clear() issuing hundreds
	 * of small writes. The cleared prefix also keeps partial alerts invalid.
	 */
	if (retained_mem_clear(pxRetainedMemory) != 0)
	{
		return DFM_FAIL;
	}
	ulRetainedMemorySum = 0U;
	
	/* Reset Zephyr retention's cached prefix state. The prefix mismatch makes
	 * this return without scanning the retained data.
	 */
	return retention_is_valid(pxRetention0) == 0 ? DFM_SUCCESS : DFM_FAIL;
}

DfmResult_t xDfmRetainedMemoryPortWrite(void* pvData, unsigned int ulWriteSize, unsigned int ulWriteOffset)
{
	if ((ulWriteOffset > DFM_RETENTION_DATA_SIZE) ||
		(ulWriteSize > (DFM_RETENTION_DATA_SIZE - ulWriteOffset)))
	{
		return DFM_FAIL;
	}

	/* Keep the alert invalid while it is incomplete. SUM32 is accumulated from
	 * data already in hand, avoiding another retained-memory read before reboot.
	 */
	if (retained_mem_write(pxRetainedMemory,
			DFM_RETENTION_AREA_OFFSET + DFM_RETENTION_PREFIX_SIZE + ulWriteOffset,
			(uint8_t*)pvData, ulWriteSize) != 0)
	{
		return DFM_FAIL;
	}
	ulRetainedMemorySum = prvSum32Update(ulRetainedMemorySum,
		(const uint8_t*)pvData, ulWriteSize, ulWriteOffset);

	return DFM_SUCCESS;
}

DfmResult_t xDfmRetainedMemoryPortCommit(void)
{
	uint8_t ucSum[DFM_RETENTION_SUM_SIZE];

	sys_put_le32(ulRetainedMemorySum, ucSum);

	/* retention_write() stores SUM32 first and the validity prefix last, making
	 * the alert and its successfully written payload data visible atomically.
	 */
	if (retention_write(pxRetention0,
			DFM_RETENTION_DATA_SIZE,
			ucSum, sizeof(ucSum)) != 0)
	{
		return DFM_FAIL;
	}
	
	return DFM_SUCCESS;
}

DfmResult_t xDfmRetainedMemoryPortRead(void* pvBuffer, unsigned int ulReadSize, unsigned int ulReadOffset)
{
	if ((ulReadOffset > DFM_RETENTION_DATA_SIZE) ||
		(ulReadSize > (DFM_RETENTION_DATA_SIZE - ulReadOffset)))
	{
		return DFM_FAIL;
	}
	
	if (retention_read(pxRetention0, ulReadOffset, (uint8_t*)pvBuffer, ulReadSize) != 0)
	{
		return DFM_FAIL;
	}
	
	return DFM_SUCCESS;
}

uint32_t xDfmRetainedMemoryPortHasData(void)
{
	uint32_t ulOffset = 0U;
	uint32_t ulSum = 0U;
	uint8_t ucBuffer[CONFIG_RETENTION_BUFFER_SIZE];
	uint8_t ucStoredSum[DFM_RETENTION_SUM_SIZE];

	if (retention_is_valid(pxRetention0) != 1)
	{
		return 0U;
	}

	while (ulOffset < DFM_RETENTION_DATA_SIZE)
	{
		uint32_t ulReadSize = MIN(DFM_RETENTION_DATA_SIZE - ulOffset,
			sizeof(ucBuffer));

		if (retention_read(pxRetention0, ulOffset, ucBuffer, ulReadSize) != 0)
		{
			return 0U;
		}
		ulSum = prvSum32Update(ulSum, ucBuffer, ulReadSize, ulOffset);
		ulOffset += ulReadSize;
	}

	if (retention_read(pxRetention0, DFM_RETENTION_DATA_SIZE,
			ucStoredSum, sizeof(ucStoredSum)) != 0)
	{
		return 0U;
	}

	return ulSum == sys_get_le32(ucStoredSum) ? 1U : 0U;
}

#endif
