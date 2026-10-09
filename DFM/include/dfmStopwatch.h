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
 * @brief DFM StopWatch API
 */

#ifndef DFM_STOPWATCH_H_
#define DFM_STOPWATCH_H_

#include <stdint.h>

typedef struct {
	/* Internal timestamps and durations are stored as TraceRecorder HWTC ticks. */
	uint32_t start_time;
	uint32_t expected_duration;
	uint32_t high_watermark;
	uint32_t id; /* Starts with 1, 0 is invalid */
	uint32_t times_above;
	const char* name;
} dfmStopwatch_t;

#if ((DFM_CFG_ENABLED) >= 1)

/* PUBLIC API */

/**
 * @brief Create a stopwatch with an expected maximum duration.
 *
 * @param name Stopwatch name.
 * @param expected_max_us Expected maximum duration in microseconds.
 * Values beyond one 32-bit HWTC period saturate to UINT32_MAX ticks. Measured
 * intervals must be shorter than one HWTC period to handle counter wrap-around.
 */
dfmStopwatch_t* xDfmStopwatchCreate(const char* name, uint32_t expected_max_us);

void vDfmStopwatchBegin(dfmStopwatch_t* sw);

void vDfmStopwatchEnd(dfmStopwatch_t* sw);

void vDfmStopwatchClearAll(void);

void vDfmStopwatchPrintAll(void);

/** @return The high watermark in microseconds, or 0 if the index is invalid. */
uint32_t xDfmStopwatchHighWatermarkGet(uint32_t index);

#else

#define xDfmStopwatchCreate(name, expected_max_us) ((dfmStopwatch_t*)0)
#define vDfmStopwatchBegin(sw) 
#define vDfmStopwatchEnd(sw) 
#define vDfmStopwatchClearAll() 
#define vDfmStopwatchPrintAll() 
#define xDfmStopwatchHighWatermarkGet(index) (0)

#endif

#endif
