#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
// #include <zephyr/fs/littlefs.h>

#include <dfm.h>

#define DFM_STORAGE_PORT_ALERT_TYPE		0x34561842
#define DFM_STORAGE_PORT_PAYLOAD_TYPE	0x82713124

static char pbFileNameBuffer[64];
static char pbCurrentReadDirectory[64];
static DfmStoragePortData_t* pxStoragePortData;
static struct fs_dir_t xFsDirHandle;
// static struct fs_file_t xFileHandle;
static struct fs_dirent xLfsInfo;
static struct fs_dirent xLfsInfoInner;
static struct fs_dir_t xInnerDir;
static struct fs_dirent xStatInfo;

static uint16_t usCurrentPayloadNumber;
static uint16_t usCurrentChunkNumber;

DfmResult_t xDfmStoragePortInitialize(DfmStoragePortData_t *pxBuffer)
{
    /* Check if the alerts directory exists, handle issues with it being nonexistant*/
    int32_t result = fs_stat(CONFIG_PERCEPIO_DFM_CFG_STORAGE_PORT_FILESYSTEM_PATH "/alerts", &xStatInfo);
    if (result != 0)
    {
        if (result != -ENOENT)
        {
            return DFM_FAIL;
        }

        result = fs_mkdir(CONFIG_PERCEPIO_DFM_CFG_STORAGE_PORT_FILESYSTEM_PATH "/alerts");
        if (result != 0)
        {
            return DFM_FAIL;
        }
        result = fs_stat(CONFIG_PERCEPIO_DFM_CFG_STORAGE_PORT_FILESYSTEM_PATH "/alerts", &xStatInfo);

        if (result != 0)
        {
            return DFM_FAIL;
        }
    }

    /* Verify that "CONFIG_PERCEPIO_DFM_CFG_STORAGE_PORT_FILESYSTEM_PATH/alerts" actually is a directory */
    if (xStatInfo.type != FS_DIR_ENTRY_DIR)
    {
        return DFM_FAIL;
    }

    pxStoragePortData = pxBuffer;
    pxStoragePortData->ulInitialized = 1;
    pxStoragePortData->ulOngoingTraversal = 0;
    return DFM_SUCCESS;
}

DfmResult_t prvDfmStoragePortWrite(DfmEntryHandle_t xEntryHandle, uint32_t ulType, uint32_t ulOverwrite)
{
    (void)ulOverwrite;
    uint32_t ulBytesWritten = 0;
    uint32_t ulBytesToWrite = 0;
    int32_t lFsResult;

    struct fs_file_t xFileHandle;
    fs_file_t_init(&xFileHandle);

    uint32_t ulAlertId;
    const char* pbUniqueSessionid;
    void* pvData = (void*)xEntryHandle;


    if (pxStoragePortData == (void*)0)
    {
        return DFM_FAIL;
    }

    if (pxStoragePortData->ulInitialized == 0)
    {
        return DFM_FAIL;
    }

    if (xEntryHandle == 0)
    {
        return DFM_FAIL;
    }

    if (xDfmEntryGetSize(xEntryHandle, &ulBytesToWrite) == DFM_FAIL)
    {
        return DFM_FAIL;
    }

    if (ulBytesToWrite == 0)
    {
        return DFM_FAIL;
    }

    if ((ulType != DFM_STORAGE_PORT_ALERT_TYPE) && (ulType != DFM_STORAGE_PORT_PAYLOAD_TYPE))
    {
        return DFM_FAIL;
    }

    (void)xDfmEntryGetAlertId(xEntryHandle, &ulAlertId);
    (void)xDfmEntryGetSessionId(xEntryHandle, &pbUniqueSessionid);

    /* Verify that the folder of the alert in question actually exists */
    snprintf(pbFileNameBuffer, sizeof(pbFileNameBuffer), CONFIG_PERCEPIO_DFM_CFG_STORAGE_PORT_FILESYSTEM_PATH "/alerts/%s_%lu", pbUniqueSessionid, ulAlertId);
    lFsResult = fs_stat(pbFileNameBuffer, &xStatInfo);
    if (lFsResult != 0)
    {
        return DFM_FAIL;
    }

    memset(pbFileNameBuffer, 0, sizeof(pbFileNameBuffer));
    if (ulType == DFM_STORAGE_PORT_ALERT_TYPE)
    {
        snprintf(pbFileNameBuffer, sizeof(pbFileNameBuffer), CONFIG_PERCEPIO_DFM_CFG_STORAGE_PORT_FILESYSTEM_PATH "/alerts/%s_%lu/header", pbUniqueSessionid, ulAlertId);
    }
    else
    {
        uint16_t usPayloadNum;
        uint16_t usChunkIndex;
        uint16_t usEntryType;
        (void)xDfmEntryGetEntryId(xEntryHandle, &usPayloadNum);
        (void)xDfmEntryGetChunkIndex(xEntryHandle, &usChunkIndex);
        (void)xDfmEntryGetType(xEntryHandle, &usEntryType);

        if (usEntryType == DFM_ENTRY_TYPE_PAYLOAD_HEADER)
        {
            usChunkIndex = 0;
        }
        snprintf(pbFileNameBuffer, sizeof(pbFileNameBuffer), CONFIG_PERCEPIO_DFM_CFG_STORAGE_PORT_FILESYSTEM_PATH "/alerts/%s_%lu/%d_%d", pbUniqueSessionid, ulAlertId, usPayloadNum, usChunkIndex);
    }

    lFsResult = fs_open(&xFileHandle, pbFileNameBuffer, (0x00000000 | FS_O_WRITE | FS_O_CREATE));
    if (lFsResult != 0)
    {
        return DFM_FAIL;
    }

    ulBytesWritten = fs_write(&xFileHandle, pvData, ulBytesToWrite);
    if (ulBytesWritten != ulBytesToWrite)
    {
        fs_close(&xFileHandle);
        return DFM_FAIL;
    }

    fs_close(&xFileHandle);
    return DFM_SUCCESS;
}

DfmResult_t xDfmStoragePortStoreAlert(DfmEntryHandle_t xEntryHandle, uint32_t ulOverwrite)
{
    int32_t lFsResult;
    const char* pbUniqueSessionid;
    uint32_t ulDfmAlertId;

    memset(pbFileNameBuffer, 0, sizeof(pbFileNameBuffer));
    (void)xDfmEntryGetSessionId(xEntryHandle, &pbUniqueSessionid);
    (void)xDfmEntryGetAlertId(xEntryHandle, &ulDfmAlertId);

    snprintf(pbFileNameBuffer, sizeof(pbFileNameBuffer), CONFIG_PERCEPIO_DFM_CFG_STORAGE_PORT_FILESYSTEM_PATH "/alerts/%s_%lu", pbUniqueSessionid, ulDfmAlertId);
    lFsResult = fs_mkdir(pbFileNameBuffer);
    if (lFsResult != 0)
    {
        return DFM_FAIL;
    }

    return prvDfmStoragePortWrite(xEntryHandle, DFM_STORAGE_PORT_ALERT_TYPE, ulOverwrite);
}

DfmResult_t xDfmStoragePortStorePayloadChunk(DfmEntryHandle_t xEntryHandle, uint32_t ulOverwrite)
{
    (void)ulOverwrite;
    return prvDfmStoragePortWrite(xEntryHandle, DFM_STORAGE_PORT_PAYLOAD_TYPE, 0);
}

/**
 * @brief
 * Remove a directory recursively, required in case an alert cannot be read.
 *
 * @param pbDirectoryPath The path to the directory which should be removed
 *
 * @retval DFM_FAIL Failure
 * @retval DFM_SUCCESS Success
*/
DfmResult_t prvDfmRemoveAlertDir(const char* pbDirectoryPath)
{
    char pbInnerFilePath[64];
    int32_t lFsResult;

    fs_dir_t_init(&xInnerDir);
    lFsResult = fs_opendir(&xInnerDir, pbDirectoryPath);
    if (lFsResult != 0)
    {
        return DFM_FAIL;
    }

    while (1)
    {
        lFsResult = fs_readdir(&xInnerDir, &xLfsInfoInner);
        if (lFsResult < 0)
        {
            return DFM_FAIL;
        }
        else if (lFsResult == 0 && xLfsInfoInner.name[0] == 0)
        {
            fs_closedir(&xInnerDir);
            fs_unlink(pbDirectoryPath);
            return DFM_SUCCESS;
        }

        if (strncmp(xLfsInfoInner.name, ".", sizeof(xLfsInfoInner.name)) == 0 || strncmp(xLfsInfoInner.name, "..", sizeof(xLfsInfoInner.name)) == 0) {
            continue;
        }

        snprintf(pbInnerFilePath, sizeof(pbInnerFilePath), "%s/%s", pbDirectoryPath, xLfsInfoInner.name);
        lFsResult = fs_unlink(pbInnerFilePath);
        if (lFsResult != 0)
        {
            return DFM_FAIL;
        }
    }
}

DfmResult_t xDfmStoragePortGetAlert(void* pvBuffer, uint32_t ulBufferSize)
{
    struct fs_file_t xFileHandle;
    DfmEntryHandle_t xEntryHandle = 0;
    int32_t lFsResult;

    if (pxStoragePortData == (void*)0)
    {
        return DFM_FAIL;
    }

    if (pxStoragePortData->ulInitialized == 0)
    {
        return DFM_FAIL;
    }

    if (pvBuffer == (void*)0)
    {
        return DFM_FAIL;
    }

    if (ulBufferSize == 0)
    {
        return DFM_FAIL;
    }

    /* If no ongoing traversal is ongoing, traverse the alert folder to check for new alerts to send */
    if (pxStoragePortData->ulOngoingTraversal == 0)
    {
        fs_dir_t_init(&xFsDirHandle);
        lFsResult = fs_opendir(&xFsDirHandle, CONFIG_PERCEPIO_DFM_CFG_STORAGE_PORT_FILESYSTEM_PATH "/alerts");
        if (lFsResult != 0)
        {
            return DFM_FAIL;
        }
        pxStoragePortData->ulOngoingTraversal = 1;
    }
    else
    {
        /* If we have handled a previous alert, it's time to remove the empty folder */
        prvDfmRemoveAlertDir(pbCurrentReadDirectory);
    }

    while (xDfmEntryCreateAlertFromBuffer(&xEntryHandle) == DFM_FAIL)
    {
        do {
            lFsResult = fs_readdir(&xFsDirHandle, &xLfsInfo);

            /* We're done, no more alerts available*/
            if (lFsResult < 0 || (lFsResult == 0 && xLfsInfo.name[0] == 0))
            {
                pxStoragePortData->ulOngoingTraversal = 0;
                fs_closedir(&xFsDirHandle);
                return DFM_FAIL;
            }
            /* Ignore . and .., these are not valid alerts */
        } while (strncmp(xLfsInfo.name, ".", sizeof(xLfsInfo.name)) == 0 || strncmp(xLfsInfo.name, "..", sizeof(xLfsInfo.name)) == 0);

        /* Current alert directory path */
        snprintf(pbCurrentReadDirectory, sizeof(pbCurrentReadDirectory), CONFIG_PERCEPIO_DFM_CFG_STORAGE_PORT_FILESYSTEM_PATH "/alerts/%s", xLfsInfo.name);

        snprintf(pbFileNameBuffer, sizeof(pbFileNameBuffer), "%s/header", pbCurrentReadDirectory);
        /* Maybe TODO: Data size shouldn't be bigger than chunk size (handled during the creation of the alert) */

        fs_file_t_init(&xFileHandle);
        lFsResult = fs_open(&xFileHandle, pbFileNameBuffer, (0x00000000 | FS_O_READ));
        if (lFsResult != 0)
        {
            return DFM_FAIL;
        }

        lFsResult = fs_read(&xFileHandle, pvBuffer, ulBufferSize);
        /* TODO: Might want to stat the file before reading it to verify size, however, a negative number will be returned upon error, which we currently rely on*/
        fs_close(&xFileHandle);
        if (lFsResult < 0)
        {
            prvDfmRemoveAlertDir(pbCurrentReadDirectory);
        }

        //prvPrintDataAsHex((uint8_t*)pvBuffer, lFsResult);
    }

    usCurrentPayloadNumber = 1;
    usCurrentChunkNumber = 0;

    return DFM_SUCCESS;
}

DfmResult_t xDfmStoragePortGetPayloadChunk(char* szSessionId, uint32_t ulAlertId, void* pvBuffer, uint32_t ulBufferSize)
{
    int32_t lFsResult;
    struct fs_file_t xChunkFileHandle;

    do {
        /* Try to retreive the next chunk */
        snprintf(pbFileNameBuffer, sizeof(pbFileNameBuffer), CONFIG_PERCEPIO_DFM_CFG_STORAGE_PORT_FILESYSTEM_PATH "/alerts/%s_%lu/%d_%d", szSessionId, ulAlertId, usCurrentPayloadNumber, usCurrentChunkNumber);
        if (fs_stat(pbFileNameBuffer, &xStatInfo) == 0)
            break;

        /* Chunk didn't exist, try to retrieve the next payload number */
        usCurrentPayloadNumber++;
        usCurrentChunkNumber = 0;
        snprintf(pbFileNameBuffer, sizeof(pbFileNameBuffer), CONFIG_PERCEPIO_DFM_CFG_STORAGE_PORT_FILESYSTEM_PATH "/alerts/%s_%lu/%d_%d", szSessionId, ulAlertId, usCurrentPayloadNumber, usCurrentChunkNumber);
        if (fs_stat(pbFileNameBuffer, &xStatInfo) == 0)
            break;

        /* All payloads have been retrieved */
        return DFM_FAIL;
    } while (false);

    fs_file_t_init(&xChunkFileHandle);
    lFsResult = fs_open(&xChunkFileHandle, pbFileNameBuffer, (0x00000000 | FS_O_READ));
    /* This should not happen considering that the file was stattable */
    if (lFsResult != 0)
    {
        /* Returing DFM_FAIL here will be enough since the directory will be cleaned up on the next xDfmGetAlert */
        return DFM_FAIL;
    }

    lFsResult = fs_read(&xChunkFileHandle, pvBuffer, ulBufferSize);
    if (lFsResult < 0)
    {
        fs_close(&xChunkFileHandle);
        return DFM_FAIL;
    }

    fs_close(&xChunkFileHandle);
    usCurrentChunkNumber++;
    return DFM_SUCCESS;
}

DfmResult_t xDfmStoragePortStoreSession(void* pvData, uint32_t ulSize)
{
    (void)pvData;
    (void)ulSize;

	if (pxStoragePortData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxStoragePortData->ulInitialized == 0)
	{
		return DFM_FAIL;
	}

	/* Check if parameters are valid. */
	if(pvData != (void*)0)
	{
		return DFM_FAIL;
	}

	if (ulSize == 0)
	{
		return DFM_FAIL;
	}

	/* TODO: Store session outside FCB (or in another FCB with just one small entry) */

	return DFM_FAIL;
}

DfmResult_t xDfmStoragePortGetSession(void* pvBuffer, uint32_t ulBufferSize)
{
    (void)pvBuffer;
    (void)ulBufferSize;

	if (pxStoragePortData == (void*)0)
	{
		return DFM_FAIL;
	}

	if (pxStoragePortData->ulInitialized == 0)
	{
		return DFM_FAIL;
	}

	return DFM_FAIL;
}
