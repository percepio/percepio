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
 * @brief DFM Entry
 */

#ifndef DFM_SESSION_H
#define DFM_SESSION_H

#include <stdint.h>
#include <dfmTypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#if ((DFM_CFG_ENABLED) >= 1)

/**
 * @defgroup dfm_session_api DFM Session API
 * @ingroup dfm_apis
 * @{
 */


/**
 * @brief Session system data
 */
typedef struct DfmSessionData
{
	uint32_t ulInitialized;

	uint32_t ulEnabled;
	
	DfmStorageStrategy_t xStorageStrategy;
	DfmCloudStrategy_t xCloudStrategy;
	DfmSessionIdStrategy_t xSessionIdStrategy;

	/* The product number */
	uint32_t ulProduct;

	/* DFM Status */
	uint32_t ulDfmStatus;

	/* Firmware Version with a pre-calculated 4-byte aligned size with room for zero termination */
	char cFirmwareVersionBuffer[DFM_FIRMWARE_VERSION_MAX_LEN];

	char cDeviceNameBuffer[DFM_DEVICE_NAME_MAX_LEN]; /* The device name */

	char cUniqueSessionIdBuffer[DFM_SESSION_ID_MAX_LEN]; /* The Session ID */

	uint32_t ulAlertCounter;
} DfmSessionData_t;

/**
 * @internal Initializes the Session system
 *
 * @param[in] pxBuffer Session system buffer.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmSessionInitialize(DfmSessionData_t* pxBuffer);

/**
 * @brief Enable DFM.
 *
 * @param[in] ulOverride Flag indicating if it should override any previous disable calls. Setting this to 1 means that if for some reason it was decided to disable DFM on this device and it was stored to Flash, this Enable attempt will not do anything.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmSessionEnable(uint32_t ulOverride);

/**
 * @brief Disable DFM.
 * 
 * @param[in] ulRemember Flag indicating if the Disable should be stored in permanent storage. Setting this to 1 means that any reboots of the device will remember the "Disabled" flag and xDfmEnable(1) must be called to override and re-enable DFM.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmSessionDisable(uint32_t ulRemember);

/**
 * @brief Query DFM enabled/disabled.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
uint32_t ulDfmSessionIsEnabled(void);

/**
 * @brief Set Session status.
 * 
 * @param[in] ulStatus Status.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmSessionSetStatus(uint32_t ulStatus);

/**
 * @brief Get Session status.
 * 
 * @param[out] pulStatus Pointer to status.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmSessionGetStatus(uint32_t* pulStatus);

/**
 * @brief Generate new Alert Id.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmSessionGenerateNewAlertId(void);

/**
 * @brief Get Alert Id.
 *
 * @param[out] pulAlertId Pointer to Alert Id.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmSessionGetAlertId(uint32_t* pulAlertId);

/**
 * @brief Get product.
 *
 * @param[out] pulProduct Pointer to product.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmSessionGetProduct(uint32_t* pulProduct);

/**
 * @brief Get firmware version.
 *
 * @param[out] pszFirmwareVersionBuffer Pointer to firmware version.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmSessionGetFirmwareVersion(char **pszFirmwareVersionBuffer);

/**
 * @brief Set device name
 *
 * @param[in] szDeviceName Device name.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmSessionSetDeviceName(const char* szDeviceName);

/**
 * @brief Get device name
 *
 * @param[out] pszDeviceName Pointer to device name.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmSessionGetDeviceName(const char** pszDeviceName);

/**
 * @brief Get Session Id.
 *
 * @param[in] pszUniqueSessionId Pointer to Session Id.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmSessionGetUniqueSessionId(char** pszUniqueSessionId);

/**
 * @brief Set Cloud strategy.
 *
 * @param[in] xStrategy Cloud strategy.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmSessionSetCloudStrategy(DfmCloudStrategy_t xStrategy);

/**
 * @brief Get Cloud strategy.
 *
 * @param[out] pxStrategy Pointer to Cloud strategy.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmSessionGetCloudStrategy(DfmCloudStrategy_t* pxStrategy);

/**
 * @brief Set Storage strategy.
 *
 * @param[in] xStrategy Storage strategy.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmSessionSetStorageStrategy(DfmStorageStrategy_t xStrategy);

/**
 * @brief Get Storage strategy.
 *
 * @param[out] pxStrategy Pointer to Storage strategy.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmSessionGetStorageStrategy(DfmStorageStrategy_t* pxStrategy);

/**
 * @brief Set Session Id strategy.
 *
 * @param[in] xStrategy Session Id strategy.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmSessionSetSessionIdStrategy(DfmSessionIdStrategy_t xStrategy);

/**
 * @brief Get Session Id strategy.
 *
 * @param[out] pxStrategy Pointer to Session Id strategy.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmSessionGetSessionIdStrategy(DfmSessionIdStrategy_t* pxStrategy);

/** @} */

#else

/* Dummy defines */
#define xDfmSessionEnable(ulOverride) (DFM_FAIL)
#define xDfmSessionDisable() (DFM_FAIL)
#define ulDfmSessionIsEnabled() (0)
#define xDfmSessionSetStatus(ulStatus) (DFM_FAIL)
#define xDfmSessionGetStatus(pulStatus) (DFM_FAIL)
#define xDfmSessionAssignAlertId(pulAlertId) (DFM_FAIL)
#define xDfmSessionGetProduct(pulProduct) (DFM_FAIL)
#define xDfmSessionGetFirmwareVersion(pszFirmwareVersionBuffer) (DFM_FAIL)
#define xDfmSessionGetDeviceName(pszDeviceName) (DFM_FAIL)
#define xDfmSessionGetUniqueSessionId(pszUniqueSessionId) (DFM_FAIL)
#define xDfmSessionSetAlertStrategy(xStrategy) (DFM_FAIL)
#define xDfmSessionGetAlertStrategy(pxStrategy) (DFM_FAIL)
#define xDfmSessionSetCloudStrategy(xStrategy) (DFM_FAIL)
#define xDfmSessionGetCloudStrategy(pxStrategy) (DFM_FAIL)
#define xDfmSessionSetStorageStrategy(xStrategy) (DFM_FAIL)
#define xDfmSessionGetStorageStrategy(pxStrategy) (DFM_FAIL)
#define xDfmSessionSetSessionIdStrategy(xStrategy) (DFM_FAIL)
#define xDfmSessionGetSessionIdStrategy(pxStrategy) (DFM_FAIL)
#define xDfmSessionSetDeviceNameStrategy(xStrategy) (DFM_FAIL)
#define xDfmSessionGetDeviceNameStrategy(pxStrategy) (DFM_FAIL)

#endif

#ifdef __cplusplus
}
#endif

#endif
