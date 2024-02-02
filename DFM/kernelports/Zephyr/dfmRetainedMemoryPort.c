/*
 * Percepio DFM v2.1.0
 * Copyright 2023 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * DFM Retained Memory Port for Zephyr
 */

#include <dfm.h>

#if (defined(DFM_CFG_ENABLED) && (DFM_CFG_ENABLED >= 1)) && (defined(DFM_CFG_RETAINED_MEMORY) && (DFM_CFG_RETAINED_MEMORY >= 1))
	
#include <zephyr/retention/retention.h>

static const struct device *pxRetention0 = DEVICE_DT_GET(DT_NODELABEL(retention0));

DfmResult_t xDfmRetainedMemoryPortInitialize(DfmRetainedMemoryPortData_t* pxBuffer)
{
	(void)pxBuffer;

	return DFM_SUCCESS;
}

DfmResult_t xDfmRetainedMemoryPortClear(void)
{
	if (retention_clear(pxRetention0) != 0)
	{
		return DFM_FAIL;
	}
	
	return DFM_SUCCESS;
}

DfmResult_t xDfmRetainedMemoryPortWrite(void* pvData, unsigned int ulWriteSize, unsigned int ulWriteOffset)
{
	if (retention_size(pxRetention0) < ulWriteOffset + ulWriteSize)
	{
		/* There's not enough space left in the retention memory area */
		return DFM_FAIL;
	}

	if (retention_write(pxRetention0, ulWriteOffset, (uint8_t*)pvData, ulWriteSize) != 0)
	{
		return DFM_FAIL;
	}
	
	return DFM_SUCCESS;
}

DfmResult_t xDfmRetainedMemoryPortRead(void* pvBuffer, unsigned int ulReadSize, unsigned int ulReadOffset)
{
	if (retention_size(pxRetention0) < ulReadOffset + ulReadSize)
	{
		/* There's not enough space left in the retention memory area */
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
	return (uint32_t)retention_is_valid(pxRetention0);
}

#endif
