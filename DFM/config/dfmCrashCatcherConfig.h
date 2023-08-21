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
 * @brief DFM CrashCatcher integration config
 */

#ifndef DFM_CRASH_CATCHER_CONFIG_H
#define DFM_CRASH_CATCHER_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief How many bytes to dump from the stack (relative to current stack pointer).
 */
#define DFM_CFG_STACKDUMP_SIZE 300

/**
 * @brief If this is set to 1 it will attempt to also save a trace with the Alert. This requires the Percepio Trace Recorder to also be included in the project.
 */
#define DFM_CFG_CRASH_ADD_TRACE	(1)

#ifdef __cplusplus
}
#endif

#endif /* DFM_CRASH_CATCHER_CONFIG_H */
