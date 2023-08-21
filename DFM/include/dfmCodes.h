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
 * @brief DFM Codes
 */

/**
 * @defgroup dfm_codes DFM Codes
 * @ingroup dfm_apis
 * @{
 */

#ifndef DFM_CODES_H
#define DFM_CODES_H
/* Alert Types */
/* The following Alert Types are published and will not change. */
#define DFM_TYPE_STACK_CHK_FAILED (8) /* Stack corrupted */
#define DFM_TYPE_HEARTBEAT (7) /* Heartbeat failure */
#define DFM_TYPE_BAD_MESSAGE (6) /* Invalid/bad message received */
#define DFM_TYPE_OVERLOAD (5) /* CPU Overload */
#define DFM_TYPE_MANUAL_TRACE (4) /* User invoked alert */
#define DFM_TYPE_HARDFAULT (3) /* Hard Fault */
#define DFM_TYPE_MALLOC_FAILED (2) /* Malloc Failed */
#define DFM_TYPE_ASSERT_FAILED (1) /* Assert Failed */


/* Symptoms */
/* The following Symptoms are published and will not change. */
#define DFM_SYMPTOM_HARDFAULT_REASON (8)
#define DFM_SYMPTOM_ARM_SCB_FCSR (7) /* CFSR (misspelled) */
#define DFM_SYMPTOM_STACKPTR (6) /* Stack Pointer */
#define DFM_SYMPTOM_PC (5) /* PC */
#define DFM_SYMPTOM_LINE (4) /* Line */
#define DFM_SYMPTOM_FUNCTION (3) /* Function */
#define DFM_SYMPTOM_FILE (2) /* File */
#define DFM_SYMPTOM_CURRENT_TASK (1) /* Current Task */

/** @} */

#endif
