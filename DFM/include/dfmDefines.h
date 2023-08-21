/*
 * Percepio DFM v2.0.0
 * Copyright 2023 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief DFM Defines
 */

/**
 * @defgroup dfm_defines DFM Defines
 * @ingroup dfm_apis
 * @{
 */

#ifndef DFM_DEFINES_H
#define DFM_DEFINES_H

#define DFM_SESSION_ID_MAX_LEN (32)

#define DFM_PAYLOAD_DESCRIPTION_MAX_LEN (16)

#define DFM_SUCCESS (0u)
#define DFM_FAIL (1u)

#define DFM_ENABLED (0x15152725UL)
#define DFM_DISABLED (0x71289203UL)

/* DFM Status Codes */
#define DFM_STATUS_CODE_NOT_INITIALIZED							(0u)
#define DFM_STATUS_CODE_OK										(1u)
#define DFM_STATUS_CODE_TOO_SMALL_PAYLOAD_SIZE					(2u)
#define DFM_STATUS_CODE_TOO_SMALL_FIRMWARE_VERSION_BUFFER		(3u)
#define DFM_STATUS_CODE_MAX_SYMPTOMS_EXCEEDED					(4u)
#define DFM_STATUS_CODE_NONVOLATILE_DATA_ALREADY_PRESENT		(5u)
#define DFM_STATUS_CODE_GET_UNIQUE_SESSION_ID_FAILED			(6u)

/** @} */

#endif
