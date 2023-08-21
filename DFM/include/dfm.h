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
 * @brief DFM API
 */

#ifndef DFM_H
#define DFM_H

#ifdef __cplusplus
extern "C" {
#endif


#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>

#include <dfmDefines.h>
#include <dfmTypes.h>
#include <dfmConfig.h>

/**
 * @brief A callback type that is used by DFM to retrieve user supplied identifiers
 *
 * Example:
 * DfmResult_t xGetUniqueSessionID(char cUniqueSessionIdBuffer[], uint32_t ulSize, uint32_t* pulBytesWritten)
 * {
 *     // This function will write a device-unique SessionId to cUniqueSessionIdBuffer.
 *
 *     // *** Some writing that must not exceed ulSize bytes... ***
 *
 *     *pulBytesWritten = <string_length>;
 *     return DFM_SUCCESS;
 * }
 */
typedef DfmResult_t (*DfmUserCallback_t)(char cBuffer[], uint32_t ulSize, uint32_t* pulBytesWritten);

/**
 * @brief This user-supplied function callback is set in xDfmInitialize().
 * It is supposed to provide a device unique "Session ID".
 * This is basically a "restart counter" that is combined with the "Device ID"
 * to generate unique "alert keys" for each alert.
 *
 * THIS MUST NOT BE A CONSTANT DUMMY STRING, EVEN FOR BASIC TESTING.
 * DUPLICATE ALERT KEYS ARE IGNORED BY DEVALERT.
 *
 * The alert key follows the pattern "DevAlert/DeviceID/SessionID/AlertCounter"
 * and must be unique across all alerts from all devices, over all time.
 * The Alert Counter is reset when the device is restarted.
 *
 * For production use, it is NOT ADVICED to use a RANDOM number, like below,
 * since this may repeat.
 *
 * The recommended practice is to use one of these methods:
 * - The current time ("wall-clock" time), e.g. in seconds since a certain date
 * - A restart counter, stored in non-volatile storage.
 *
 * If using the current time as Session ID, seconds is sufficient resolution
 * unless a restart and re-initialization may occur faster than that.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
extern DfmUserCallback_t xDfmUserGetUniqueSessionID;

/**
 * @brief This user-supplied function callback is set in xDfmInitialize().
 * It is suppoed to provide the "Device ID" for each alert.
 * This is combined with the "Session ID" to generate unique "alert keys" for each alert.
 *
 * THIS MUST NOT BE A CONSTANT DUMMY STRING. DUPLICATE ALERT KEYS ARE IGNORED BY DEVALERT.
 *
 * If using AWS IoT Core, note that Device ID does not need to match the Thing Name
 * (i.e. clientcredentialIOT_THING_NAME in /demos/include/aws_clientcredentials.h).
 * It is good practice and recommended to have matching Thing Name and Device ID.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
extern DfmUserCallback_t xDfmUserGetDeviceName;

/* 64-bit aligned */
#define DFM_DEVICE_NAME_MAX_LEN ((((DFM_CFG_DEVICE_NAME_MAX_LEN) + 7) / 8) * 8)

/* 64-bit aligned with room for zero termination */
#define DFM_FIRMWARE_VERSION_MAX_LEN ((((DFM_CFG_FIRMWARE_VERSION_MAX_LEN) + 1 + 7) / 8) * 8)

/* 64-bit aligned with room for zero termination */
#define DFM_DESCRIPTION_MAX_LEN ((((DFM_CFG_DESCRIPTION_MAX_LEN) + 1 + 7) / 8) * 8)

#include <dfmKernelPort.h>
#include <dfmEntry.h>
#include <dfmAlert.h>
#include <dfmSession.h>
#include <dfmStorage.h>
#include <dfmCloud.h>

#include <dfmCodes.h>

#ifndef DFM_CFG_ENABLED
#error DFM_CFG_ENABLED not set in dfmConfig.h!
#endif

#ifndef DFM_CFG_FIRMWARE_VERSION_MAX_LEN
#error DFM_CFG_FIRMWARE_VERSION_MAX_LEN not set in dfmConfig.h!
#endif

#if DFM_CFG_PRODUCTID == 0
#error DFM_CFG_PRODUCTID not set in dfmConfig.h!
#endif

#ifndef DFM_ERROR_PRINT
#define DFM_ERROR_PRINT(msg) 
#endif

#if (DFM_CFG_ENABLE_DEBUG_PRINT == 1)
#define DFM_DEBUG_PRINT(msg) DFM_ERROR_PRINT(msg)
#else
#define DFM_DEBUG_PRINT(msg) ((void)(msg))
#endif

#if ((DFM_CFG_ENABLED) == 1)

/**
 * @defgroup dfm_alert_apis DFM Alert API
 * @ingroup dfm_apis
 * @{
 */

/*******************************************************************************
 * Versions
 *
 * 1) Initial version
 * 2) Added ulProduct to DFM header
 * 3) Refactored DFM
 ******************************************************************************/
#define DFM_VERSION_INITIAL 1
#define DFM_VERSION_WITH_PRODUCT 2
#define DFM_VERSION_2_0 3

#define DFM_VERSION DFM_VERSION_2_0

#if (DFM_FIRMWARE_VERSION_MAX_LEN > 255)
#error "Firmware Max Length cannot be larger than 255"
#endif

/**
 * @brief DFM data
 */
typedef struct DfmData
{
	uint32_t ulDfmInitialized;

	DfmSessionData_t xSessionData;
	DfmKernelPortData_t xKernelPortData;
	DfmStorageData_t xStorageData;
	DfmCloudData_t xCloudData;
	DfmAlertData_t xAlertData;
	DfmEntryData_t xEntryData;
} DfmData_t;

/**
 * @internal Initializes the entire DFM system
 *
 * @param[in] xGetUniqueSessionID This user-defined function provides the "Session ID".
 * This is basically a "restart counter" that is combined with the "Device ID"
 * to generate unique "alert keys" for each alert.
 *
 * THIS MUST NOT BE A CONSTANT DUMMY STRING, EVEN FOR BASIC TESTING.
 * DUPLICATE ALERT KEYS ARE IGNORED BY DEVALERT.
 *
 * The alert key follows the pattern "DevAlert/DeviceID/SessionID/AlertCounter"
 * and must be unique across all alerts from all devices, over all time.
 * The Alert Counter is reset when the device is restarted.
 *
 * For production use, it is NOT ADVICED to use a RANDOM number, like below,
 * since this may repeat.
 *
 * The recommended practice is to use one of these methods:
 * - The current time ("wall-clock" time), e.g. in seconds since a certain date
 * - A restart counter, stored in non-volatile storage.
 *
 * If using the current time as Session ID, seconds is sufficient resolution
 * unless a restart and re-initialization may occur faster than that.
 *
 * @param[in] xGetDeviceName This user-defined function provides the "Device ID" for each alert.
 * This is combined with the "Session ID" to generate unique "alert keys" for each alert.
 *
 * THIS MUST NOT BE A CONSTANT DUMMY STRING. DUPLICATE ALERT KEYS ARE IGNORED BY DEVALERT.
 *
 * If using AWS IoT Core, note that Device ID does not need to match the Thing Name
 * (i.e. clientcredentialIOT_THING_NAME in /demos/include/aws_clientcredentials.h).
 * It is good practice and recommended to have matching Thing Name and Device ID.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmInitialize(DfmUserCallback_t xGetUniqueSessionID, DfmUserCallback_t xGetDeviceName);

/**
 * @brief Is DFM initialized?
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
uint32_t ulDfmIsInitialized(void);

/**
 * @brief Enable DFM
 *
 * @param[in] ulOverride Flag indicating if it should override any previous disable calls. Setting this to 1 means that if for some reason it was decided to disable DFM on this device and it was stored to Flash, this Enable attempt will not do anything.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
#define xDfmEnable(ulOverride) xDfmSessionEnable(ulOverride)

/**
 * @brief Disable DFM
 *
 * @param[in] ulRemember Flag indicating if the Disable should be stored in permanent storage. Setting this to 1 means that any reboots of the device will remember the "Disabled" flag and xDfmEnable(1) must be called to override and re-enable DFM.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
#define xDfmDisable(ulRemember) xDfmSessionDisable(ulRemember)

/**
 * @brief Is DFM enabled?
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
#define ulDfmIsEnabled() ulDfmSessionIsEnabled()

 /** @} */

#else

/* Dummy defines */
#define xDfmInitialize(p,fv) (DFM_FAIL)
#define ulDfmIsInitialized() (0)
#define xDfmEnable(ulOverride) (DFM_FAIL)
#define xDfmDisable() (DFM_FAIL)
#define ulDfmIsEnabled() (0)
	
#endif

#ifdef __cplusplus
}
#endif

#endif /* DFM_H */
