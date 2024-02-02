/*
 * Percepio DFM v2.1.0
 * Copyright 2023 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief DFM Retained Memory Port for Zephyr
 */

#ifndef DFM_RETAINED_MEMORY_PORT_H
#define DFM_RETAINED_MEMORY_PORT_H

#include <stdint.h>
#include <dfmConfig.h>
#include <dfmTypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#if (defined(DFM_CFG_ENABLED) && (DFM_CFG_ENABLED >= 1)) && (defined(DFM_CFG_RETAINED_MEMORY) && (DFM_CFG_RETAINED_MEMORY >= 1))

typedef struct DfmRetainedMemoryPortData
{
    void* dummy;
} DfmRetainedMemoryPortData_t;

/**
 * @brief Initialize Retained Memory port
 *
 * @param[in] pxBuffer Retained Memory port buffer.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmRetainedMemoryPortInitialize(DfmRetainedMemoryPortData_t* pxBuffer);

/**
 * @brief Clears all alerts in Retained Memory
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmRetainedMemoryPortClear(void);

/**
 * @brief Write data to Retained Memory
 *
 * @param[in] pvData Pointer to data.
 * @param[in] ulWriteSize Data size.
 * @param[in] ulWriteOffset Write offset.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmRetainedMemoryPortWrite(void* pvData, unsigned int ulWriteSize, unsigned int ulWriteOffset);

/**
 * @brief Read data from Retained Memory
 *
 * @param[in] pvBuffer Buffer to read to.
 * @param[in] ulReadSize Read size.
 * @param[in] ulReadOffset Read offset.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmRetainedMemoryPortRead(void* pvBuffer, unsigned int ulReadSize, unsigned int ulReadOffset);

/**
 * @brief Checks if there is data in Retained Memory
 *
 * @retval 1 There is valid data
 * @retval 0 There is no valid data
 */
uint32_t xDfmRetainedMemoryPortHasData(void);

#endif

#ifdef __cplusplus
}
#endif

#endif