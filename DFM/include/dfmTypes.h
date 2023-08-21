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
 * @brief DFM Types
 */

#ifndef DFM_TYPES_H
#define DFM_TYPES_H

/**
 * @defgroup dfm_types DFM Codes
 * @ingroup dfm_apis
 * @{
 */

typedef uint32_t DfmResult_t;

typedef void* DfmAlertHandle_t;
typedef void* DfmEntryHandle_t;

typedef enum DfmEntryType
{
	DFM_ENTRY_TYPE_ALERT = 0x1512,
	DFM_ENTRY_TYPE_PAYLOAD_HEADER = 0x4618,
	DFM_ENTRY_TYPE_PAYLOAD = 0x8371,
} DfmEntryType_t;

typedef enum DfmStorageStrategy
{
	DFM_STORAGE_STRATEGY_IGNORE,		/** Never store alerts/payloads */
	DFM_STORAGE_STRATEGY_OVERWRITE,		/** Overwrite old alerts/payloads if full */
	DFM_STORAGE_STRATEGY_SKIP,			/** Skip if full */
} DfmStorageStrategy_t;

typedef enum DfmCloudStrategy
{
	DFM_CLOUD_STRATEGY_OFFLINE,			/** Will not attempt to send alerts/payloads */
	DFM_CLOUD_STRATEGY_ONLINE,			/** Will attempt to send alerts/payloads */
} DfmCloudStrategy_t;

typedef enum DfmSessionIdStrategy
{
	DFM_SESSIONID_STRATEGY_ONSTARTUP,
	DFM_SESSIONID_STRATEGY_ONALERT,
} DfmSessionIdStrategy_t;

/** @} */

#endif
