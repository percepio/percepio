/*
 * Percepio DFM v2.0.0
 * Copyright 2023 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * DFM Session
 */

#include <dfm.h>

#if ((DFM_CFG_ENABLED) >= 1)

typedef struct DfmSessionStorage
{
	uint32_t ulVersion;
	uint32_t ulEnabled;
} DfmSessionStorage_t;

#define DFM_SESSION_STORAGE_VERSION 1

#define DFM_SESSION_STORAGE_DUMMY_VALUE 0x93105271UL

#if ((DFM_DEVICE_NAME_MAX_LEN) < 8)
#error Minimum DFM_CFG_DEVICE_NAME_MAX_LEN size is 8!
#endif

static DfmSessionData_t* pxDfmSessionData = (void*)0;

DfmResult_t prvGetSessionStorageVersion(DfmSessionStorage_t* pxSessionStorage, uint32_t* pulVersion);
DfmResult_t prvGetSessionStorageEnabled(DfmSessionStorage_t* pxSessionStorage, uint32_t* pulEnabled);

DfmResult_t xDfmSessionInitialize(DfmSessionData_t* pxBuffer)
{
	uint32_t i;
	uint32_t ulBytesWritten;

	if (pxBuffer == (void*)0)
	{
		return DFM_FAIL;
	}

	pxDfmSessionData = pxBuffer;

	pxDfmSessionData->ulAlertCounter = 0;

	pxDfmSessionData->ulProduct = DFM_CFG_PRODUCTID;
	
	pxDfmSessionData->xStorageStrategy = DFM_CFG_STORAGE_STRATEGY;
	pxDfmSessionData->xCloudStrategy = DFM_CFG_CLOUD_STRATEGY;
	pxDfmSessionData->xSessionIdStrategy = DFM_CFG_SESSIONID_STRATEGY;

	pxDfmSessionData->ulDfmStatus = DFM_STATUS_CODE_OK;

	/* We disable it here */
	pxDfmSessionData->ulEnabled = DFM_DISABLED;

	if (pxDfmSessionData->xSessionIdStrategy == DFM_SESSIONID_STRATEGY_ONSTARTUP)
	{
		/* Verify that the user supplied callback has been set */
		if (xDfmUserGetUniqueSessionID == 0)
		{
			return DFM_FAIL;
		}

		if (xDfmUserGetUniqueSessionID(pxDfmSessionData->cUniqueSessionIdBuffer, DFM_SESSION_ID_MAX_LEN, &ulBytesWritten) == DFM_FAIL) /*cstat !MISRAC2012-Rule-14.3_b The User implementation used for MISRA always returns DFM_SUCCESS so this check will never be true. In a real system that will not be the case.*/
		{
			return DFM_FAIL;
		}

		if (pxDfmSessionData->cUniqueSessionIdBuffer[0] == (char)0)
		{
			return DFM_FAIL;
		}
	}
	else
	{
		pxDfmSessionData->cUniqueSessionIdBuffer[0] = (char)0;
	}

	pxDfmSessionData->cDeviceNameBuffer[0] = (char)0;

	for (i = 0; i < (uint32_t)DFM_CFG_FIRMWARE_VERSION_MAX_LEN; i++)
	{
		pxDfmSessionData->cFirmwareVersionBuffer[i] = (char)(DFM_CFG_FIRMWARE_VERSION)[i];

		if (pxDfmSessionData->cFirmwareVersionBuffer[i] == (char)0)
		{
			break;
		}
	}

	pxDfmSessionData->ulInitialized = 1;

	return DFM_SUCCESS;
}

DfmResult_t xDfmSessionEnable(uint32_t ulOverride)
{
	uint8_t cSessionStorageBuffer[8] = {0}; /* This must be large enough to hold all previous versions of DfmSessionStorage_t */
	uint32_t ulStoredEnabledValue = DFM_SESSION_STORAGE_DUMMY_VALUE;

	if (pxDfmSessionData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmSessionData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (ulDfmIsInitialized() == 0UL)
	{
		return DFM_FAIL;
	}

	/* Already enabled? */
	if (pxDfmSessionData->ulEnabled == DFM_ENABLED)
	{
		return DFM_SUCCESS;
	}

	/* Is a previous session stored? */
	if (xDfmStorageGetSession(cSessionStorageBuffer, sizeof(cSessionStorageBuffer)) == DFM_SUCCESS)
	{
		if (prvGetSessionStorageEnabled((DfmSessionStorage_t*)cSessionStorageBuffer, &ulStoredEnabledValue) == DFM_FAIL) /*cstat !MISRAC2012-Rule-11.3 We use an untyped buffer to retrieve the session data since we can't know what version and size it might have been stored in the past. The stored Session data might be larger than the current DfmSessionStorage_t*/
		{
			/* Unexpected error */
			ulStoredEnabledValue = DFM_SESSION_STORAGE_DUMMY_VALUE;
		}
	}

	if ((ulStoredEnabledValue == DFM_SESSION_STORAGE_DUMMY_VALUE) || ((ulOverride == 1UL) && (ulStoredEnabledValue == DFM_DISABLED)))
	{
		/* Couldn't read stored value or we override a disabled value */
		((DfmSessionStorage_t*)cSessionStorageBuffer)->ulVersion = DFM_SESSION_STORAGE_VERSION; /*cstat !MISRAC2012-Rule-11.3 We use an untyped buffer to retrieve the session data since we can't know what version and size it might have been stored in the past. The stored Session data might be larger than the current DfmSessionStorage_t*/
		((DfmSessionStorage_t*)cSessionStorageBuffer)->ulEnabled = DFM_ENABLED; /*cstat !MISRAC2012-Rule-11.3 We use an untyped buffer to retrieve the session data since we can't know what version and size it might have been stored in the past. The stored Session data might be larger than the current DfmSessionStorage_t*/

		(void)xDfmStorageStoreSession(cSessionStorageBuffer, sizeof(DfmSessionStorage_t)); /* Attempt to store the session info. We can't really do anything if it fails. */

		ulStoredEnabledValue = DFM_ENABLED;
	}

	if (ulStoredEnabledValue == DFM_DISABLED)
	{
		/* We are expressly disabled and not overriding */
		return DFM_FAIL;
	}

	pxDfmSessionData->ulEnabled = DFM_ENABLED;

	return DFM_SUCCESS;
}

DfmResult_t xDfmSessionDisable(uint32_t ulRemember)
{
	uint8_t cSessionStorageBuffer[8] = { 0 }; /* This must be large enough to hold all previous versions of DfmSessionStorage_t */
	uint32_t ulStoredEnabledValue = DFM_SESSION_STORAGE_DUMMY_VALUE;

	if (pxDfmSessionData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmSessionData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	/* Is a previous session stored? */
	if (xDfmStorageGetSession(cSessionStorageBuffer, sizeof(cSessionStorageBuffer)) == DFM_SUCCESS)
	{
		if (prvGetSessionStorageEnabled((DfmSessionStorage_t*)cSessionStorageBuffer, &ulStoredEnabledValue) == DFM_FAIL) /*cstat !MISRAC2012-Rule-11.3 We use an untyped buffer to retrieve the session data since we can't know what version and size it might have been stored in the past. The stored Session data might be larger than the current DfmSessionStorage_t*/
		{
			/* Unexpected error */
			ulStoredEnabledValue = DFM_SESSION_STORAGE_DUMMY_VALUE;
		}
	}

	if (ulStoredEnabledValue != DFM_DISABLED)
	{
		/* We didn't find disabled */
		((DfmSessionStorage_t*)cSessionStorageBuffer)->ulVersion = DFM_SESSION_STORAGE_VERSION; /*cstat !MISRAC2012-Rule-11.3 We use an untyped buffer to retrieve the session data since we can't know what version and size it might have been stored in the past. The stored Session data might be larger than the current DfmSessionStorage_t*/
		((DfmSessionStorage_t*)cSessionStorageBuffer)->ulEnabled = DFM_DISABLED; /*cstat !MISRAC2012-Rule-11.3 We use an untyped buffer to retrieve the session data since we can't know what version and size it might have been stored in the past. The stored Session data might be larger than the current DfmSessionStorage_t*/

		if (ulRemember != 0UL)
		{
			(void)xDfmStorageStoreSession(cSessionStorageBuffer, sizeof(DfmSessionStorage_t)); /* Attempt to store the session info. We can't really do anything if it fails. */
		}
	}

	pxDfmSessionData->ulEnabled = DFM_DISABLED;

	return DFM_SUCCESS;
}

uint32_t ulDfmSessionIsEnabled()
{
	if (pxDfmSessionData == (void*)0)
	{
		return 0;
	}

	if (pxDfmSessionData->ulInitialized == 0UL)
	{
		return 0;
	}

	if (ulDfmIsInitialized() == 0UL)
	{
		return 0;
	}

	return (uint32_t)(pxDfmSessionData->ulEnabled == DFM_ENABLED);
}

DfmResult_t xDfmSessionGetUniqueSessionId(char **pszUniqueSessionId)
{
	uint32_t ulBytesWritten = 0UL;

	if (pxDfmSessionData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmSessionData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (pszUniqueSessionId == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmSessionData->cUniqueSessionIdBuffer[0] == (char)0)
	{
		if (pxDfmSessionData->xSessionIdStrategy == DFM_SESSIONID_STRATEGY_ONALERT)
		{
			/* Verify that the user supplied callback has been set */
			if (xDfmUserGetUniqueSessionID == 0)
			{
				return DFM_FAIL;
			}

			/* Attempt to get a valid session id. Reserve last buffer slot for null termination. */
			if (xDfmUserGetUniqueSessionID(pxDfmSessionData->cUniqueSessionIdBuffer, (uint32_t)(DFM_SESSION_ID_MAX_LEN) - 1UL, &ulBytesWritten) == DFM_FAIL) /*cstat !MISRAC2012-Rule-14.3_b The User implementation used for MISRA always returns DFM_SUCCESS so this check will never be true. In a real system that will not be the case.*/
			{
				return DFM_FAIL;
			}

			if (pxDfmSessionData->cUniqueSessionIdBuffer[0] == (char)0)
			{
				return DFM_FAIL;
			}

			if (ulBytesWritten > (uint32_t)(DFM_SESSION_ID_MAX_LEN) - 1UL)
			{
				/* Wrote outside buffer? */
				return DFM_FAIL;
			}

			/* Make sure we have null termination */
			pxDfmSessionData->cUniqueSessionIdBuffer[ulBytesWritten] = (char)0;
		}
		else
		{
			/* This should have been set on startup! */
			return DFM_FAIL;
		}
	}

	*pszUniqueSessionId = pxDfmSessionData->cUniqueSessionIdBuffer;

	return DFM_SUCCESS;
}

DfmResult_t xDfmSessionSetDeviceName(const char* szDeviceName)
{
	if (pxDfmSessionData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmSessionData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (szDeviceName == (void*)0)
	{
		return DFM_FAIL;
	}

	/* Copy the device name string, but make sure we leave the last byte for zero termination */
	for (int i = 0; i < DFM_DEVICE_NAME_MAX_LEN - 1; i++)
	{
		pxDfmSessionData->cDeviceNameBuffer[i] = szDeviceName[i];

		/* Break at zero termination */
		if (szDeviceName[i] == (char)0)
		{
			break;
		}
	}

	pxDfmSessionData->cDeviceNameBuffer[DFM_DEVICE_NAME_MAX_LEN - 1] = (char)0;

	return DFM_SUCCESS;
}

DfmResult_t xDfmSessionGetDeviceName(const char** pszDeviceName)
{
	uint32_t ulBytesWritten = 0UL;

	if (pxDfmSessionData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmSessionData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (pszDeviceName == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmSessionData->cDeviceNameBuffer[0] == (char)0)
	{
		/* Verify that the user supplied callback has been set */
		if (xDfmUserGetDeviceName == 0)
		{
			return DFM_FAIL;
		}

		/* Attempt to get a valid device name. Reserve last buffer slot for null termination. */
		if (xDfmUserGetDeviceName(pxDfmSessionData->cDeviceNameBuffer, (uint32_t)(DFM_DEVICE_NAME_MAX_LEN) - 1UL, &ulBytesWritten) == DFM_FAIL) /*cstat !MISRAC2012-Rule-14.3_b The User implementation used for MISRA always returns DFM_SUCCESS so this check will never be true. In a real system that will not be the case.*/
		{
			return DFM_FAIL;
		}

		if (pxDfmSessionData->cDeviceNameBuffer[0] == (char)0)
		{
			return DFM_FAIL;
		}

		if (ulBytesWritten > (uint32_t)(DFM_DEVICE_NAME_MAX_LEN) - 1UL)
		{
			/* Wrote too much, no room for null termination! */
			return DFM_FAIL;
		}

		/* Make sure we have null termination */
		pxDfmSessionData->cDeviceNameBuffer[ulBytesWritten] = (char)0;
	}
		
	*pszDeviceName = pxDfmSessionData->cDeviceNameBuffer;

	return DFM_SUCCESS;
}

DfmResult_t xDfmSessionGenerateNewAlertId(void)
{
	if (pxDfmSessionData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmSessionData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	/* alertIDs start at 1 */
	pxDfmSessionData->ulAlertCounter++;

	return DFM_SUCCESS;
}

DfmResult_t xDfmSessionGetAlertId(uint32_t* pulAlertId)
{
	if (pxDfmSessionData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmSessionData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (pulAlertId == (void*)0)
	{
		return DFM_FAIL;
	}
	
	*pulAlertId = pxDfmSessionData->ulAlertCounter;

	return DFM_SUCCESS;
}

DfmResult_t xDfmSessionGetProduct(uint32_t* pulProduct)
{
	if (pxDfmSessionData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmSessionData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (pulProduct == (void*)0)
	{
		return DFM_FAIL;
	}

	*pulProduct = pxDfmSessionData->ulProduct;

	return DFM_SUCCESS;
}

DfmResult_t xDfmSessionGetFirmwareVersion(char** pszFirmwareVersion)
{
	if (pxDfmSessionData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmSessionData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (pszFirmwareVersion == (void*)0)
	{
		return DFM_FAIL;
	}

	*pszFirmwareVersion = pxDfmSessionData->cFirmwareVersionBuffer;
	return DFM_SUCCESS;
}

DfmResult_t xDfmSessionSetStatus(uint32_t ulStatus)
{
	if (pxDfmSessionData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmSessionData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (pxDfmSessionData->ulDfmStatus != DFM_STATUS_CODE_OK)
	{
		return DFM_FAIL;
	}

	pxDfmSessionData->ulDfmStatus = ulStatus;

	return DFM_SUCCESS;
}

DfmResult_t xDfmSessionGetStatus(uint32_t* pulStatus)
{
	if (pxDfmSessionData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmSessionData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (pulStatus == (void*)0)
	{
		return DFM_FAIL;
	}

	*pulStatus = pxDfmSessionData->ulDfmStatus;

	return DFM_SUCCESS;
}

DfmResult_t xDfmSessionSetCloudStrategy(DfmCloudStrategy_t xStrategy)
{
	if (pxDfmSessionData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmSessionData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	pxDfmSessionData->xCloudStrategy = xStrategy;

	return DFM_SUCCESS;
}

DfmResult_t xDfmSessionGetCloudStrategy(DfmCloudStrategy_t* pxStrategy)
{
	if (pxDfmSessionData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmSessionData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (pxStrategy == (void*)0)
	{
		return DFM_FAIL;
	}

	*pxStrategy = pxDfmSessionData->xCloudStrategy;

	return DFM_SUCCESS;
}

DfmResult_t xDfmSessionSetStorageStrategy(DfmStorageStrategy_t xStrategy)
{
	if (pxDfmSessionData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmSessionData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	pxDfmSessionData->xStorageStrategy = xStrategy;

	return DFM_SUCCESS;
}

DfmResult_t xDfmSessionGetStorageStrategy(DfmStorageStrategy_t* pxStrategy)
{
	if (pxDfmSessionData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmSessionData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (pxStrategy == (void*)0)
	{
		return DFM_FAIL;
	}

	*pxStrategy = pxDfmSessionData->xStorageStrategy;

	return DFM_SUCCESS;
}

DfmResult_t xDfmSessionSetSessionIdStrategy(DfmSessionIdStrategy_t xStrategy)
{
	if (pxDfmSessionData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmSessionData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	pxDfmSessionData->xSessionIdStrategy = xStrategy;

	return DFM_SUCCESS;
}

DfmResult_t xDfmSessionGetSessionIdStrategy(DfmSessionIdStrategy_t* pxStrategy)
{
	if (pxDfmSessionData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxDfmSessionData->ulInitialized == 0UL)
	{
		return DFM_FAIL;
	}

	if (pxStrategy == (void*)0)
	{
		return DFM_FAIL;
	}

	*pxStrategy = pxDfmSessionData->xSessionIdStrategy;

	return DFM_SUCCESS;
}

DfmResult_t prvGetSessionStorageVersion(DfmSessionStorage_t* pxSessionStorage, uint32_t* pulVersion)
{
	if (pxSessionStorage->ulVersion != 1UL)
	{
		return DFM_FAIL;
	}

	*pulVersion = pxSessionStorage->ulVersion;

	return DFM_SUCCESS;
}

DfmResult_t prvGetSessionStorageEnabled(DfmSessionStorage_t* pxSessionStorage, uint32_t* pulEnabled)
{
	if (pxSessionStorage->ulVersion != 1UL)
	{
		return DFM_FAIL;
	}

	*pulEnabled = pxSessionStorage->ulEnabled;

	return DFM_SUCCESS;
}

#endif
