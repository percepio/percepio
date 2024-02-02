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
 * @brief DFM Configuration
 */

#ifndef DFM_CONFIG_H
#define DFM_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Global flag used to completely exclude all DFM functionality from compilation
 */
#define DFM_CFG_ENABLED CONFIG_PERCEPIO_DFM

/**
 * @brief The firmware version. This needs to be set to differentiate the alerts between versions.
 */
#define DFM_CFG_FIRMWARE_VERSION CONFIG_PERCEPIO_DFM_CFG_FIRMWARE_VERSION

/**
 * @brief An identifier of the product type.
 */
#define DFM_CFG_PRODUCTID CONFIG_PERCEPIO_DFM_CFG_PRODUCTID

/* Enable diagnostic messages from DFM_DEBUG(...). Will use DFM_ERROR to output debug information. */
#define DFM_CFG_ENABLE_DEBUG_PRINT CONFIG_PERCEPIO_DFM_CFG_ENABLE_DEBUG_PRINT

/* Add your serial console print string function here (full printf not needed, only "print") */
#define DFM_CFG_PRINT(msg) printk("%s", msg)

/* This will be called for errors. Point this to a suitable print function. This will also be used for DFM_DEBUG_PRINT messages. */
#define DFM_ERROR_PRINT(msg) DFM_CFG_PRINT(msg)

/**
 * @brief The maximum size of a "chunk" that will be stored or sent.
 */
#define DFM_CFG_MAX_PAYLOAD_CHUNK_SIZE CONFIG_PERCEPIO_DFM_CFG_MAX_PAYLOAD_CHUNK_SIZE

/**
 * @brief The maximum length of the device name.
 */
#define DFM_CFG_DEVICE_NAME_MAX_LEN CONFIG_PERCEPIO_DFM_CFG_DEVICE_NAME_MAX_LEN

/**
 * @brief The maximum number of payloads that can be attached to an alert.
 */
#define DFM_CFG_MAX_PAYLOADS CONFIG_PERCEPIO_DFM_CFG_MAX_PAYLOADS

/**
 * @brief The max number of symptoms for each alert
 */
#define DFM_CFG_MAX_SYMPTOMS CONFIG_PERCEPIO_DFM_CFG_MAX_SYMPTOMS

/**
 * @brief The max firmware version string length
 */
#define DFM_CFG_FIRMWARE_VERSION_MAX_LEN CONFIG_PERCEPIO_DFM_CFG_FIRMWARE_VERSION_MAX_LEN

/**
 * @brief The max description string length
 */
#define DFM_CFG_DESCRIPTION_MAX_LEN CONFIG_PERCEPIO_DFM_CFG_DESCRIPTION_MAX_LEN

/**
 * @brief A value that will be used to create a delay between transfers. Was necessary in certain situations.
 */
#define DFM_CFG_DELAY_BETWEEN_SEND CONFIG_PERCEPIO_DFM_CFG_DELAY_BETWEEN_SEND

/**
 * @brief Enables the Retained Memory feature. Requires a RetainedMemoryPort to be implemented for the kernel/hardware.
 */
#define DFM_CFG_RETAINED_MEMORY CONFIG_PERCEPIO_DFM_CFG_RETAINED_MEMORY

/**
 * @brief The strategy used for storing alerts/payload. Possible values are:
 *	DFM_STORAGE_STRATEGY_IGNORE			Never store alerts/payloads
 *	DFM_STORAGE_STRATEGY_OVERWRITE		Overwrite old alerts/payloads if full
 *	DFM_STORAGE_STRATEGY_SKIP			Skip if full
 */
#if CONFIG_PERCEPIO_DFM_CFG_SELECTED_STORAGE_STRATEGY_IGNORE == 1
	#define DFM_CFG_STORAGE_STRATEGY DFM_STORAGE_STRATEGY_IGNORE
#elif CONFIG_PERCEPIO_DFM_CFG_SELECTED_STORAGE_STRATEGY_OVERWRITE == 1
	#define DFM_CFG_STORAGE_STRATEGY DFM_STORAGE_STRATEGY_OVERWRITE
#elif CONFIG_PERCEPIO_DFM_CFG_SELECTED_STORAGE_STRATEGY_SKIP == 1
	#define DFM_CFG_STORAGE_STRATEGY DFM_STORAGE_STRATEGY_SKIP
#endif

/**
 * @brief The strategy used for sending alerts/payload. Possible values are:
*	DFM_CLOUD_STRATEGY_OFFLINE			Will not attempt to send alerts/payloads
*	DFM_CLOUD_STRATEGY_ONLINE			Will attempt to send alerts/payloads
*/
#if CONFIG_PERCEPIO_DFM_CFG_SELECTED_CLOUD_STRATEGY_ONLINE == 1
	#define DFM_CFG_CLOUD_STRATEGY DFM_CLOUD_STRATEGY_ONLINE
#elif CONFIG_PERCEPIO_DFM_CFG_SELECTED_CLOUD_STRATEGY_OFFLINE == 1
	#define DFM_CFG_CLOUD_STRATEGY DFM_CLOUD_STRATEGY_OFFLINE
#endif

/**
 * @brief The strategy used for acquiring the unique session ID. Possible values are:
*	DFM_SESSIONID_STRATEGY_ONSTARTUP	Acquires the unique session ID at startup
*	DFM_SESSIONID_STRATEGY_ONALERT		Acquires the unique session ID the first time an alert is generated
*/
#if CONFIG_PERCEPIO_DFM_CFG_SELECTED_SESSIONID_STRATEGY_ONALERT == 1
	#define DFM_CFG_SESSIONID_STRATEGY DFM_SESSIONID_STRATEGY_ONALERT
#elif CONFIG_PERCEPIO_DFM_CFG_SELECTED_SESSIONID_STRATEGY_ONSTARTUP == 1
	#define DFM_CFG_SESSIONID_STRATEGY DFM_SESSIONID_STRATEGY_ONSTARTUP
#endif

/**
 * @brief The strategy used for acquiring the device name. Possible values are:
* 	DFM_DEVICE_NAME_STRATEGY_SKIP		Some devides don't know their names, skip it
*	DFM_DEVICE_NAME_STRATEGY_ONDEVICE	This device knows its' name, get it
*/
/* TODO: Is this one even used? */
#define DFM_CFG_DEVICENAME_STRATEGY DFM_DEVICE_NAME_STRATEGY_ONDEVICE

#if CONFIG_PERCEPIO_DFM_CFG_ENABLE_COREDUMPS == 1
	#define DFM_CFG_ENABLE_COREDUMPS 1
#else
	#define DFM_CFG_ENABLE_COREDUMPS 0
#endif

#ifdef __cplusplus
}
#endif

#endif /* DFM_CONFIG_H */
