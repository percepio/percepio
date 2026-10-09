/*
 * Percepio DFM
 * Copyright 2026 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * DFM utility functions
 */

#ifndef DFM_UTILITY_H
#define DFM_UTILITY_H

#include <stddef.h>
#include <dfm.h>

#if ((DFM_CFG_ENABLED) >= 1)

#if ((DFM_CFG_CRASH_ADD_TRACE) >= 1)
#include <trcRecorder.h>
#endif

/* Caller-saved Cortex-M registers captured immediately when DFM_TRAP starts. */
typedef struct DfmTrapCallerSavedContext
{
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r12;
} DfmTrapCallerSavedContext_t;

/*
 * Compile-time guard for the fixed byte offsets used by the assembly
 * implementations. This declaration generates no code or storage. A build
 * error here means that DfmTrapCallerSavedContext_t no longer matches the
 * layout assumed by the assembly; update both together rather than removing
 * this check.
 */
typedef char DfmTrapCallerSavedContextLayoutMustMatchAssembly[
	(offsetof(DfmTrapCallerSavedContext_t, r0) == 0U) &&
	(offsetof(DfmTrapCallerSavedContext_t, r1) == 4U) &&
	(offsetof(DfmTrapCallerSavedContext_t, r2) == 8U) &&
	(offsetof(DfmTrapCallerSavedContext_t, r3) == 12U) &&
	(offsetof(DfmTrapCallerSavedContext_t, r12) == 16U) &&
	(sizeof(DfmTrapCallerSavedContext_t) == 20U) ? 1 : -1];

extern volatile DfmTrapCallerSavedContext_t dfmTrapCallerSavedContext;

/*
 * Implemented by each Cortex-M coredump integration as a naked assembly
 * function. It copies r0-r3 and r12 to dfmTrapCallerSavedContext before a C
 * prologue or DFM_TRAP argument evaluation can change them. Any temporary
 * stack use and register changes are balanced inside the function, before
 * control returns to compiler-generated code.
 */
void dfmTrapCaptureCallerSavedContext(void);

/*
 * Capture the caller-saved registers first, then store the DFM_TRAP metadata
 * with the normal C stack pointer. Passing the metadata directly to the
 * capture function would overwrite r0-r3 before they could be recorded.
 */
#define DFM_TRAP_SAVE_ARGS(_alertType, _message, _file, _line, _restart_flag) \
	do {                                                                        \
		dfmTrapCaptureCallerSavedContext();                                       \
		dfmTrapInfo.alertType = (int)(_alertType);                                \
		dfmTrapInfo.message   = (char *)(_message);                               \
		dfmTrapInfo.file      = (char *)(_file);                                  \
		dfmTrapInfo.line      = (int)(_line);                                     \
		dfmTrapInfo.restart   = (int)(_restart_flag);                             \
	} while (0)

/* Used for __FILE__ macro to extract the filename from the full path. */
const char * szDfmGetFileNameFromPath(const char *szPath);

uint32_t ulDfmCalculateChecksum(const char *ptr, size_t maxlen);

uint16_t usDfmCalculateCrc16Ccitt(uint16_t usSeed, const uint8_t* pucData, uint32_t ulSize);

/* Returns DFM_SUCCESS if both symptoms could be added. */
DfmResult_t xDfmAddFileAndLineSymptoms(DfmAlertHandle_t xAlertHandle, const char *szFileName, int iLineNumber);

#if ((DFM_CFG_CRASH_ADD_TRACE) >= 1)
/* Acquires the event buffer from the Trace Recorder and adds as a payload. */
DfmResult_t vDfmAddTracePayload(DfmAlertHandle_t xAlertHandle);
#endif

#endif /* DFM_CFG_ENABLED */

#endif /* DFM_UTILITY_H */
