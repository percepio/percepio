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
 * @brief DFM Alert API
 */

#ifndef DFM_ALERT_H
#define DFM_ALERT_H

#include <stdint.h>
#include <dfmTypes.h>
#include <dfmConfig.h>

#ifdef __cplusplus
extern "C" {
#endif

#if ((DFM_CFG_ENABLED) >= 1)

/**
 * @defgroup dfm_alert_apis DFM Alert API
 * @ingroup dfm_apis
 * @{
 */

#ifndef DFM_CFG_USE_TRACE_RECORDER
#define DFM_CFG_USE_TRACE_RECORDER 0
#endif

#ifndef DFM_CFG_MAX_SYMPTOMS
#error DFM_CFG_MAX_SYMPTOMS not set in dfmConfig.h!
#endif

#ifndef DFM_CFG_DESCRIPTION_MAX_LEN
#error DFM_CFG_DESCRIPTION_MAX_LEN not set in dfmConfig.h!
#endif

#if (DFM_FIRMWARE_VERSION_MAX_LEN > 255)
#error "Firmware Max Length cannot be larger than 255"
#endif

#if (DFM_DESCRIPTION_MAX_LEN > 255)
#error "Description Max Length cannot be larger than 255"
#endif

#if ((DFM_CFG_MAX_SYMPTOMS) <= 0)
#error "DFM_CFG_MAX_SYMPTOMS invalid"
#endif /*((DFM_CFG_MAX_SYMPTOMS) < 0) */

#define DFM_PAYLOAD_DESCRIPTION_MAX_LEN (16)

/**
 * @brief Callback type used when retrieving all alert data
 */
typedef DfmResult_t (*DfmAlertEntryCallback_t)(DfmEntryHandle_t xEntryHandle);

/**
 * @brief Alert symptom definition
 */
typedef struct
{
	uint32_t ulId;			/* Symptom ID */
	uint32_t ulValue;				/* The Symptom value */
} DfmAlertSymptom_t;

/**
 * @brief Alert payload definition
 */
typedef struct
{
	void* pvData;
	uint32_t ulSize;
	char cDescriptionBuffer[DFM_PAYLOAD_DESCRIPTION_MAX_LEN];
} DfmAlertPayload_t;

/**
 * @brief Alert header
 */
typedef struct
{
	uint8_t ucStartMarkers[4];		/* The DFM start marker, must be 0x50, 0x44, 0x66, 0x6D ("PDfm") */

	uint16_t usEndianness;			/* The endianness checker, assign to 0x0FF0 */

	uint8_t ucVersion;				/* The version of the DFM subsystem, 0 for not enabled */
	uint8_t ucFirmwareVersionSize;	/* The maximum length of cFirmwareVersionBuffer */
	uint8_t ucMaxSymptoms;			/* The maximum number of symptoms, initialized to DFM_CFG_MAX_SYMPTOMS */
	uint8_t ucSymptomCount;			/* The number of registered symptoms. */
	uint8_t ucDescriptionSize;		/* The size of the description field, can be 0 for no description included */

	uint8_t ucReserved0;			/* Reserved for future use */

	uint32_t ulProduct;				/* The product code, can be 0 to indicate the default product. */

	uint32_t ulAlertType;			/* The alert type */

	DfmAlertSymptom_t xSymptoms[DFM_CFG_MAX_SYMPTOMS]; /* The symptoms */

	char cFirmwareVersionBuffer[DFM_FIRMWARE_VERSION_MAX_LEN]; /* Size is 4-byte aligned and with room for zero termination */

	char cAlertDescription[DFM_DESCRIPTION_MAX_LEN]; /* Size is 4-byte aligned and with room for zero termination */

	uint8_t ucEndMarkers[4];		/* The DFM start marker, must be 0x6D, 0x66, 0x44, 0x50 ("mfDP") */

	uint32_t ulChecksum;			/* Checksum on the whole thing, 0 for not enabled */
} DfmAlert_t;

/**
 * @brief Struct containing the Alert header and payload info
 */
typedef struct DfmAlertData
{
	uint32_t ulInitialized;
	DfmAlert_t xAlert;
	DfmAlertPayload_t xPayloads[DFM_CFG_MAX_PAYLOADS]; /* The payloads */
	uint32_t ulPayloadCount;
} DfmAlertData_t;

extern DfmAlertData_t* pxDfmAlertData;

/**
 * @internal Initializes the Alert system
 *
 * @param[in] pxBuffer Alert system buffer.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmAlertInitialize(DfmAlertData_t* pxBuffer);

/**
 * @brief Retrieve Alert version from Alert handle
 *
 * @param[in] pvSession Session data.
 * @param[out] pucVersion Pointer to version.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmAlertGetVersion(DfmAlertHandle_t xAlertHandle, uint8_t * pucVersion);

/**
 * @brief Retrieve product id from Alert handle
 *
 * @param[in] pvSession Session data.
 * @param[out] pulProduct Pointer to product id.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmAlertGetProduct(DfmAlertHandle_t xAlertHandle, uint32_t* pulProduct);

/**
 * @brief Retrieve Firmware version from Alert handle
 *
 * @param[in] xAlertHandle Alert handle.
 * @param[out] pszFirmwareVersion Pointer to firmware version.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmAlertGetFirmwareVersion(DfmAlertHandle_t xAlertHandle, const char** pszFirmwareVersion);

/**
 * @brief Reset Alert
 *
 * @param[in] xAlertHandle Alert handle.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmAlertReset(DfmAlertHandle_t xAlertHandle);

/**
 * @brief Begins creation of Alert
 *
 * @param[in] ulAlertType Type of Alert.
 * @param[in] szAlertDescription Alert description.
 * @param[out] pxAlertHandle Alert handle.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmAlertBegin(uint32_t ulAlertType, const char* szAlertDescription, DfmAlertHandle_t* pxAlertHandle);

/**
 * @brief Ends Alert creation and attempts to send, store or retain it, in that order.
 *
 * @param[in] xAlertHandle Alert handle.
 * 
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
#define xDfmAlertEnd(xAlertHandle) xDfmAlertEndCustom(xAlertHandle, DFM_ALERT_END_TYPE_ALL)

/**
 * @brief Ends Alert creation and sends, stores or retains it depending on the supplied ulEndType parameter.
 * The values can be combined to only attempt the appropriate types but the order will always be Send before Store before Retain.
 *
 * @param[in] xAlertHandle Alert handle.
 * @param[in] ulEndType The type of commits to be attempted by the function.
 * 						DFM_ALERT_END_TYPE_ALL: Send, store or retain the Alert, in that order.
 * 						DFM_ALERT_END_TYPE_SEND: Send the Alert.
 * 						DFM_ALERT_END_TYPE_STORE: Store the Alert.
 * 						DFM_ALERT_END_TYPE_RETAIN: Retain the Alert.
 * 
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmAlertEndCustom(DfmAlertHandle_t xAlertHandle, uint32_t ulEndType);

/**
 * @brief Ends Alert creation by only storing it
 *
 * @param[in] xAlertHandle Alert handle.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmAlertEndOffline(DfmAlertHandle_t xAlertHandle);

/**
 * @brief Ends Alert creation by only storing it in retained memory
 *
 * @param[in] xAlertHandle Alert handle.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmAlertEndRetainedMemory(DfmAlertHandle_t xAlertHandle);

/**
 * @brief Add Symptom to Alert
 *
 * @param[in] xAlertHandle Alert handle.
 * @param[in] ulSymptomId Symptom ID.
 * @param[in] ulValue Symptom value.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmAlertAddSymptom(DfmAlertHandle_t xAlertHandle, uint32_t ulSymptomId, uint32_t ulValue);

/**
 * @brief Get Symptom from Alert
 *
 * @param[in] xAlertHandle Alert handle.
 * @param[in] ulIndex Symptom index.
 * @param[out] pulSymptomId Symptom ID.
 * @param[out] pulValue Symptom value.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmAlertGetSymptom(DfmAlertHandle_t xAlertHandle, uint32_t ulIndex, uint32_t *pulSymptomId, uint32_t *pulValue);

/**
 * @brief Add Payload to Alert
 *
 * @param[in] xAlertHandle Alert handle.
 * @param[in] pvData Pointer to Payload.
 * @param[in] ulSize Payload size.
 * @param[in] szDescription Payload description.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmAlertAddPayload(DfmAlertHandle_t xAlertHandle, void* pvData, uint32_t ulSize, const char* szDescription);

/**
 * @brief Get Payload from Alert
 *
 * @param[in] xAlertHandle Alert handle.
 * @param[in] ulIndex Symptom index.
 * @param[out] ppvData Payload pointer.
 * @param[out] pulSize Payload size.
 * @param[out] pszDescription Payload description.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmAlertGetPayload(DfmAlertHandle_t xAlertHandle, uint32_t ulIndex, void** ppvData, uint32_t* pulSize, char **pszDescription);

/**
 * @brief Get Payload Type from Alert
 *
 * @param[in] xAlertHandle Alert handle.
 * @param[out] pulAlertType Payload type.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmAlertGetType(DfmAlertHandle_t xAlertHandle, uint32_t* pulAlertType);

/**
 * @brief Retrieve Alert description
 *
 * @param[in] xAlertHandle Alert handle.
 * @param[out] pszAlertDescription Alert description.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmAlertGetDescription(DfmAlertHandle_t xAlertHandle, const char** pszAlertDescription);

/**
* @brief Store any alerts in Retained Memory
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmAlertStoreRetainedMemory(void);

/**
* @brief Send all stored Alerts
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmAlertSendAll(void);

/**
* @brief Get all stored Alerts
 *
 * @param[in] xCallback The callback that will be called for every stored Alert/Payload Entry
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmAlertGetAll(DfmAlertEntryCallback_t xCallback);

/** @} */

#else

/* Dummy defines */
#define xDfmAlertGetVersion(xAlertHandle, pucVersion) ((void)(xAlertHandle), (void)(pucVersion), DFM_FAIL)
#define xDfmAlertGetId(xAlertHandle, pulAlertId) ((void)(xAlertHandle), (void)(pulAlertId), DFM_FAIL)
#define xDfmAlertGetProduct(xAlertHandle, pulProduct) ((void)(xAlertHandle), (void)(pulProduct), DFM_FAIL)
#define xDfmAlertGetFirmwareVersion(xAlertHandle, pszFirmwareVersion) ((void)(xAlertHandle), (void)(pszFirmwareVersion), DFM_FAIL)
#define xDfmAlertReset(xAlertHandle) ((void)(xAlertHandle), DFM_FAIL)
#define xDfmAlertBegin(ulAlertType, szAlertDescription, pxAlertHandle) ((void)(ulAlertType), (void)(szAlertDescription), (void)(pxAlertHandle), DFM_FAIL)
#define xDfmAlertEnd(xAlertHandle) ((void)(xAlertHandle), DFM_FAIL)
#define xDfmAlertEndOffline(xAlertHandle ((void)(xAlertHandle), DFM_FAIL)
#define xDfmAlertEndRetainedMemory(xAlertHandle) ((void)(xAlertHandle), DFM_FAIL)
#define xDfmAlertAddSymptom(xAlertHandle, ulSymptomId, ulValue) ((void)(xAlertHandle), (void)(ulSymptomId), (void)(ulValue), DFM_FAIL)
#define xDfmAlertGetSymptom(xAlertHandle, ulIndex, pulSymptomId, pulValue) ((void)(xAlertHandle), (void)(ulIndex), (void)(pulSymptomId), (void)(pulValue), DFM_FAIL)
#define xDfmAlertAddPayload(xAlertHandle, pvData, ulSize, szDescription) ((void)(xAlertHandle), (void)(pvData), (void)(ulSize), (void)(szDescription), DFM_FAIL)
#define xDfmAlertAddTracePayload(xAlertHandle) ((void)(xAlertHandle), DFM_FAIL)
#define xDfmAlertGetPayload(xAlertHandle, ulIndex, ppvData, pulSize, pszDescription) ((void)(xAlertHandle), (void)(ulIndex), (void)(ppvData), (void)(pulSize), (void)(pszDescription), DFM_FAIL)
#define xDfmAlertGetType(xAlertHandle, pulAlertType) ((void)(xAlertHandle), (void)(pulAlertType), DFM_FAIL)
#define xDfmAlertGetDescription(xAlertHandle, pszAlertDescription) ((void)(xAlertHandle), (void)(pszAlertDescription), DFM_FAIL)
#define xDfmAlertStoreRetainedMemory() (DFM_FAIL)
#define xDfmAlertSendAll() (DFM_FAIL)
#define xDfmAlertGetAll(xCallback) ((void)(xCallback), DFM_FAIL)

#endif

#ifdef __cplusplus
}
#endif

#endif
