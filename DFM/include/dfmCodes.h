/*
 * Percepio DFM
 * Copyright 2023-2026 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief THIS IS A GENERATED FILE - DO NOT EDIT. Contains IDs for Alerts and Symptoms based on the server database.
 */

#ifndef __DFM_CODES_H__
#define __DFM_CODES_H__

/* Alert Types */
/* The following Alert Types are published and will not change. */
#define DFM_TYPE_XYZ (8) /* User-Defined Alert */
#define DFM_TYPE_ZEPHYR_FATAL_ERROR (7) /* Zephyr Fatal Error */
#define DFM_TYPE_TASKMONITOR_ALERT (6) /* TaskMonitor alert */
#define DFM_TYPE_STACK_CHK_FAILED (5) /* Stack corruption detected */
#define DFM_TYPE_STOPWATCH (4) /* Stopwatch alert */
#define DFM_TYPE_HARDFAULT (3) /* Hard Fault */
#define DFM_TYPE_MALLOC_FAILED (2) /* Malloc Failed */
#define DFM_TYPE_ASSERT_FAILED (1) /* Assert Failed */


/* Symptoms */
/* The following Symptoms are published and will not change. */
#define DFM_SYMPTOM_ZEPHYR_FATAL_ERROR_REASON (10) /* Reason Code */
#define DFM_SYMPTOM_STOPWATCH_ID (9) /* Stopwatch ID */
#define DFM_SYMPTOM_HIGH_WATERMARK (8) /* High watermark */
#define DFM_SYMPTOM_STACKPTR (7) /* Stack Pointer */
#define DFM_SYMPTOM_CFSR (6) /* CFSR Register */
#define DFM_SYMPTOM_PC (5) /* PC */
#define DFM_SYMPTOM_LINE (4) /* Line */
#define DFM_SYMPTOM_FUNCTION (3) /* Function */
#define DFM_SYMPTOM_FILE (2) /* File */
#define DFM_SYMPTOM_CURRENT_TASK (1) /* Current Task */


#endif /* __DFM_CODES_H__ */
