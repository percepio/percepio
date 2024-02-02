#ifndef DFM_STORAGE_PORT_H
#define DFM_STORAGE_PORT_H

#include <stdint.h>
#include <dfmConfig.h>
// #include <dfmStoragePortConfig.h>
#include <dfmTypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#if (defined(DFM_CFG_ENABLED) && (DFM_CFG_ENABLED >= 1))

typedef struct DfmStoragePortData
{
    uint32_t ulInitialized;
    uint32_t ulOngoingTraversal;
} DfmStoragePortData_t;

/**
 * @brief Initialize Storage port system
 *
 * @param[in] pxBuffer Storage port system buffer.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmStoragePortInitialize(DfmStoragePortData_t *pxBuffer);

/**
 * @brief Store Session data
 *
 * @param[in] pvData Pointer to Session data.
 * @param[in] ulSize Session data size.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmStoragePortStoreSession(void* pvData, uint32_t ulSize);

/**
 * @brief Retrieve Session data
 *
 * @param[in] pvData Pointer to Session data buffer.
 * @param[in] ulSize Session data buffer size.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmStoragePortGetSession(void* pvBuffer, uint32_t ulBufferSize);

/**
 * @brief Store Alert Entry
 *
 * @param[in] xEntryHandle Entry handle.
 * @param[in] ulOverwrite Flag indicating if existing Entries should be overwritten.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmStoragePortStoreAlert(DfmEntryHandle_t xEntryHandle, uint32_t ulOverwrite);

/**
 * @brief Retrieve Alert Entry
 *
 * @param[in] pvBuffer Pointer to Alert Entry buffer.
 * @param[in] ulBufferSize Alert Entry buffer size.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmStoragePortGetAlert(void* pvBuffer, uint32_t ulBufferSize);

/**
 * @brief Store Payload chunk Entry
 *
 * @param[in] xEntryHandle Entry handle.
 * @param[in] ulOverwrite Flag indicating if existing Entries shuold be overwritten.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmStoragePortStorePayloadChunk(DfmEntryHandle_t xEntryHandle, uint32_t ulOverwrite);

/**
 * @brief Retrieve Payload chunk Entry
 *
 * @param[in] szSessionId Requested Session Id.
 * @param[in] ulAlertId Requested Alert Id.
 * @param[in] pvBuffer Pointer to Payload chunk Entry buffer.
 * @param[in] ulBufferSize Payload chunk Entry buffer size.
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmStoragePortGetPayloadChunk(char* szSessionId, uint32_t ulAlertId, void* pvBuffer, uint32_t ulBufferSize);

/**
 * @brief Remove all stored alerts
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
 */
DfmResult_t xDfmStoragePortReset(void);

#endif

#ifdef __cplusplus
}
#endif

#endif