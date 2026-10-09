/*
 * Percepio DFM
 * Copyright 2023-2026 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * DFM StopWatch
 */

#include <dfm.h>

/* Depends on TraceRecorder for timestamping */
#include <trcRecorder.h>

#if (TRC_HWTC_TYPE != TRC_FREE_RUNNING_32BIT_INCR)
#error Only hardware ports with TRC_HWTC_TYPE == TRC_FREE_RUNNING_32BIT_INCR are supported.
#endif

static uint32_t prvDfmStopwatchMicrosecondsToTicks(uint32_t duration_us)
{
	/* Convert microseconds to hardware timer ticks, rounding up so the
 	 * resulting interval is never shorter than the requested duration.
 	 */
	uint64_t ticks = ((uint64_t)duration_us * (uint64_t)(TRC_HWTC_FREQ_HZ) +
			1000000ULL - 1ULL) / 1000000ULL;

	return (ticks > (uint64_t)UINT32_MAX) ? UINT32_MAX : (uint32_t)ticks;
}

static uint32_t prvDfmStopwatchTicksToMicroseconds(uint32_t ticks)
{
	uint64_t frequency_hz = (uint64_t)(TRC_HWTC_FREQ_HZ);
	uint64_t duration_us;

	if (frequency_hz == 0ULL)
	{
		return 0;
	}

	/* Rounds to nearest integer. */
	duration_us = ((uint64_t)ticks * 1000000ULL + (frequency_hz / 2ULL)) / frequency_hz;

	return (duration_us > (uint64_t)UINT32_MAX) ? UINT32_MAX : (uint32_t)duration_us;
}

#ifndef DFM_CFG_MAX_STOPWATCHES
#define DFM_CFG_MAX_STOPWATCHES 1
#endif

dfmStopwatch_t stopwatches[DFM_CFG_MAX_STOPWATCHES];
int32_t stopwatch_count = 0;
int32_t stopwatch_enabled = 1; // Monitoring is disabled while saving alerts.

void prvStopwatchPrint(dfmStopwatch_t* sw, char* testresult);
void prvDfmPrintHeader(void);
void prvDfmStopwatchAlert(char* msg, uint32_t high_watermark_us, uint32_t stopwatch_index);

uint32_t xDfmStopwatchHighWatermarkGet(uint32_t index)
{
	if (index < (uint32_t)stopwatch_count)
	{
		return prvDfmStopwatchTicksToMicroseconds(stopwatches[index].high_watermark);
	}

	return 0;
}

dfmStopwatch_t* xDfmStopwatchCreate(const char* name, uint32_t expected_max_us)
{
	TRACE_ALLOC_CRITICAL_SECTION();
	dfmStopwatch_t* sw = (void*)0;
	uint32_t expected_max_ticks = prvDfmStopwatchMicrosecondsToTicks(expected_max_us);

	TRACE_ENTER_CRITICAL_SECTION();

	if (stopwatch_count < DFM_CFG_MAX_STOPWATCHES)
	{
		sw = &stopwatches[stopwatch_count];

		sw->expected_duration = expected_max_ticks;
		sw->name = name;
		sw->high_watermark = 0;
		sw->start_time = 0;
		sw->id = stopwatch_count + 1; /* starts with 1, id 0 denotes invalid/uninitialized. */
		sw->times_above = 0;

		stopwatch_count++;
	}
	TRACE_EXIT_CRITICAL_SECTION();

	return sw;
}

#ifdef INCLUDE_STOPWATCH_UNIT_TEST
static volatile uint32_t hwtc_count_simulated = 0;
#define DFM_CFG_HWTC_COUNT hwtc_count_simulated
#else
#define DFM_CFG_HWTC_COUNT TRC_HWTC_COUNT
#endif


void vDfmStopwatchBegin(dfmStopwatch_t* sw)
{
	if (sw != (void*)0)
	{
		sw->start_time = DFM_CFG_HWTC_COUNT;
	}
}

extern void prvAddTracePayload(void);

void vDfmStopwatchEnd(dfmStopwatch_t* sw)
{
	uint32_t end_time = DFM_CFG_HWTC_COUNT;

	if (sw != (void*)0)
	{
		/* Overflow in HWTC_COUNT is OK. Say that start_time is 0xFFFFFFFF
		 * and end_time is 1, then we get (1 - 0xFFFFFFFF) = 2.
		 * This since 0xFFFFFFFF equals -1 (signed) and 1 - (-1)) = 2 */
		uint32_t duration = end_time - sw->start_time;

		/* Alert if new highest value is found, assuming it is above expected_duration (provides a lower threshold) */
		if (duration > sw->high_watermark)
		{
			sw->high_watermark = duration;

			if (duration > sw->expected_duration)
			{
				uint32_t high_watermark_us = prvDfmStopwatchTicksToMicroseconds(sw->high_watermark);
				uint32_t expected_duration_us = prvDfmStopwatchTicksToMicroseconds(sw->expected_duration);

				sw->times_above++;

				snprintf(cDfmPrintBuffer, sizeof(cDfmPrintBuffer), "Stopwatch %u reached %u us (exp max: %u us)" LNBR, (unsigned int)sw->id, (unsigned int)high_watermark_us, (unsigned int)expected_duration_us);
				DFM_CFG_PRINT(cDfmPrintBuffer);

				prvDfmStopwatchAlert(cDfmPrintBuffer, high_watermark_us, sw->id);
			}
		}
	}
}

void prvStopwatchPrint(dfmStopwatch_t* sw, char* testresult)
{
	if (sw != (void*)0)
	{
		uint32_t high_watermark_us = prvDfmStopwatchTicksToMicroseconds(sw->high_watermark);
		uint32_t expected_duration_us = prvDfmStopwatchTicksToMicroseconds(sw->expected_duration);

		if (sw->name == (void*)0)
		{
			sw->name = "NULL";
		}
		snprintf(cDfmPrintBuffer, sizeof(cDfmPrintBuffer), "%12u, %-14s %9u %9u %9u %s" LNBR, (unsigned int)sw->id, sw->name, (unsigned int)high_watermark_us, (unsigned int)expected_duration_us, (unsigned int)sw->start_time, testresult);
		DFM_CFG_PRINT(cDfmPrintBuffer);
	}
	else
	{
		DFM_CFG_PRINT("Stopwatch is NULL (ERROR!)" LNBR);
	}
}

void prvDfmPrintHeader(void)
{
	snprintf(cDfmPrintBuffer, sizeof(cDfmPrintBuffer), "%12s, %-14s %9s %9s %9s" LNBR, "Stopwatch ID", "Name", "High us", "Exp us", "Last Tick");
	DFM_CFG_PRINT(cDfmPrintBuffer);
}

void vDfmStopwatchPrintAll(void)
{
	prvDfmPrintHeader();
	for (int i = 0; i < DFM_CFG_MAX_STOPWATCHES; i++)
	{
		if (stopwatches[i].id != 0)
		{
			prvStopwatchPrint(&stopwatches[i], "");
		}
	}
}

void vDfmStopwatchClearAll(void)
{
	for (int i = 0; i < DFM_CFG_MAX_STOPWATCHES; i++)
	{
		stopwatches[i].expected_duration = 0;
		stopwatches[i].name = (void*)0;
		stopwatches[i].high_watermark = 0;
		stopwatches[i].start_time = 0;
		stopwatches[i].id = 0;
	}
	stopwatch_count = 0;
}

void prvDfmStopwatchAlert(char* msg, uint32_t high_watermark_us, uint32_t stopwatch_index)
{
	static DfmAlertHandle_t xAlertHandle;
		void* pvBuffer = (void*)0;
		uint32_t ulBufferSize = 0;
	static TraceStringHandle_t TzUserEventChannel = 0;

	if (xDfmAlertBegin(DFM_TYPE_STOPWATCH, msg, &xAlertHandle) != DFM_SUCCESS)
	{
		return;
	}

		if (TzUserEventChannel == 0)
		{
			(void)xTraceStringRegister("ALERT", &TzUserEventChannel);
		}

	(void)xTracePrint(TzUserEventChannel, msg);

		/* Stopping the tracing while sending the trace data. */
		(void)xTraceDisable();

		(void)xTraceGetEventBuffer(&pvBuffer, &ulBufferSize);
		(void)xDfmAlertAddPayload(xAlertHandle, pvBuffer, ulBufferSize, "dfm_trace.psfs");

		#ifdef DFM_SYMPTOM_HIGH_WATERMARK
	(void)xDfmAlertAddSymptom(xAlertHandle, DFM_SYMPTOM_HIGH_WATERMARK, high_watermark_us);
		#endif

		#ifdef DFM_SYMPTOM_STOPWATCH_ID
		(void)xDfmAlertAddSymptom(xAlertHandle, DFM_SYMPTOM_STOPWATCH_ID, stopwatch_index);
		#endif

		/* Assumes "cloud port" is a UART or similar, that is always available. */
		if (xDfmAlertEnd(xAlertHandle) != DFM_SUCCESS)
		{
		DFM_CFG_PRINT("DFM: xDfmAlertEnd failed." LNBR);
		}

		(void)xTraceEnable(TRC_START);
	}


#if (INCLUDE_STOPWATCH_TESTS == 1)

void prvStopwatchSetSimulatedTime(uint32_t time);
void prvStopwatchClear(dfmStopwatch_t* sw);


DfmResult_t prvPrintAndCheckStopwatch(dfmStopwatch_t* sw, uint32_t expected_hwm, uint32_t expected_st, uint32_t expected_exp_dur_us, const char* expected_name, uint32_t expected_id)
{
	DfmResult_t result = DFM_SUCCESS;
	char* status = "(OK)";

	if (sw->start_time != expected_st)
	{
		status = "ERROR (start_time)";
		result = DFM_FAIL;
	}

	if (sw->expected_duration != prvDfmStopwatchMicrosecondsToTicks(expected_exp_dur_us))
	{
		status = "ERROR (expected_duration)";
		result = DFM_FAIL;
	}

	if (sw->high_watermark != expected_hwm)
	{
		status = "ERROR (high_watermark)";
		result = DFM_FAIL;
	}

	if (sw->name != expected_name)
	{
		status = "ERROR (name)";
		result = DFM_FAIL;
	}

	if (sw->id != expected_id)
	{
		status = "ERROR (id)";
		result = DFM_FAIL;
	}

	prvStopwatchPrint(sw, status);

	return result;
}

void prvRunTests(void)
{
	int err = 0;

	char swnames[DFM_CFG_MAX_STOPWATCHES][10] = {"SW1", "SW2", "SW2", "SW4"} ;

	printf("\nTest 1: xDfmStopwatchCreate().\n");
    for (int i = 0; i < DFM_CFG_MAX_STOPWATCHES + 1; i++) // Tries creating one too many stopwatches.
    {
    	dfmStopwatch_t* sw = (void*)0;

    	sw = xDfmStopwatchCreate( swnames[i], 1000 + (i * 100));

    	printf(" Call %d: ", i);

    	if (sw == (void*)0)
    	{
    		printf("NULL ");
    		if (i >= DFM_CFG_MAX_STOPWATCHES)
    		{
    			printf("(OK)\n");
    		}
    		else
    		{
    			printf("ERROR!\n");
    			err = 1;
    		}
    	}
    	else
    	{
    		printf("Not NULL (OK)\n");
    	}
    }

    prvDfmPrintHeader();
    for (int i = 0; i < DFM_CFG_MAX_STOPWATCHES; i++) // List and verify all stopwatches
    {
    	if (prvPrintAndCheckStopwatch(&stopwatches[i], 0, 0, 1000 + (100*i), swnames[i], i+1) == DFM_FAIL)
    	{
    		err = 1;
    	}
    }

    if (err == 0)
    {
    	dfmStopwatch_t* sw;
    	vDfmStopwatchClearAll();

    	// Test: Zero time elapsed
    	printf("\nTest 2: vDfmStopwatchBegin/End - Zero time elapsed.\n");
    	prvDfmPrintHeader();
    	sw = xDfmStopwatchCreate(swnames[0], 1000);
    	prvStopwatchSetSimulatedTime(0);
    	vDfmStopwatchBegin(sw);
    	vDfmStopwatchEnd(sw);
    	prvPrintAndCheckStopwatch(sw, 0, 0, 1000, swnames[0], 1);

    	printf("\nTest 3: vDfmStopwatchClearAll();\n");
    	vDfmStopwatchClearAll();
    	prvDfmPrintHeader();
    	for (int i = 0; i < DFM_CFG_MAX_STOPWATCHES; i++)
    	{
    		prvPrintAndCheckStopwatch(&stopwatches[i], 0, 0, 0, (void*)0, 0);
    	}

    	printf("\nTest 4: Begin/End - Overflow at t=0xFFFFFFFF.\n");
    	prvDfmPrintHeader();
    	sw = xDfmStopwatchCreate(swnames[0], 2);
    	prvStopwatchSetSimulatedTime(0xFFFFFFFF);
		vDfmStopwatchBegin(sw);
		prvStopwatchSetSimulatedTime(1);
		vDfmStopwatchEnd(sw);
		prvPrintAndCheckStopwatch(sw, 2, 0xFFFFFFFF, 2, swnames[0], 1);
		vDfmStopwatchClearAll();

    	printf("\nTest 5: Begin/End from 200 to 300.\n");
    	prvDfmPrintHeader();
    	sw = xDfmStopwatchCreate(swnames[0], 1000);
    	prvStopwatchSetSimulatedTime(200);
    	vDfmStopwatchBegin(sw);
    	prvStopwatchSetSimulatedTime(300);
    	vDfmStopwatchEnd(sw);
    	prvPrintAndCheckStopwatch(sw, 100, 200, 1000, swnames[0], 1);
    	vDfmStopwatchClearAll();

		printf("\nTest 6: Begin/End with 0xFFFFFFFF duration\n");
		prvDfmPrintHeader();
		sw = xDfmStopwatchCreate(swnames[0], 0xFFFFFFFF);
		prvStopwatchSetSimulatedTime(0);
		vDfmStopwatchBegin(sw);
		prvStopwatchSetSimulatedTime(0xFFFFFFFF);
		vDfmStopwatchEnd(sw);
		prvPrintAndCheckStopwatch(sw, 0xFFFFFFFF, 0, 0xFFFFFFFF, swnames[0], 1);
		vDfmStopwatchClearAll();

		printf("\nTest 7: Using all stopwatches at the same time.\n");

		{
			dfmStopwatch_t* sw0;
			dfmStopwatch_t* sw1;
			dfmStopwatch_t* sw2;
			dfmStopwatch_t* sw3;

			sw0 = xDfmStopwatchCreate(swnames[0], 100);
			sw1 = xDfmStopwatchCreate(swnames[1], 200);
			sw2 = xDfmStopwatchCreate(swnames[2], 300);
			sw3 = xDfmStopwatchCreate(swnames[3], 400);

			prvStopwatchSetSimulatedTime(10);

			vDfmStopwatchBegin(sw0);
			vDfmStopwatchBegin(sw1);
			vDfmStopwatchBegin(sw2);
			vDfmStopwatchBegin(sw3);

			prvStopwatchSetSimulatedTime(110);
			vDfmStopwatchEnd(sw0);

			prvStopwatchSetSimulatedTime(210);
			vDfmStopwatchEnd(sw1);

			prvStopwatchSetSimulatedTime(310);
			vDfmStopwatchEnd(sw2);

			prvStopwatchSetSimulatedTime(410);
			vDfmStopwatchEnd(sw3);

			prvDfmPrintHeader();
			prvPrintAndCheckStopwatch(sw0, 100, 10, 100, swnames[0], 1);
			prvPrintAndCheckStopwatch(sw1, 200, 10, 200, swnames[1], 2);
			prvPrintAndCheckStopwatch(sw2, 300, 10, 300, swnames[2], 3);
			prvPrintAndCheckStopwatch(sw3, 400, 10, 400, swnames[3], 4);

			vDfmStopwatchClearAll();
		}
    }

    for (;;);
}

// Only intended for unit testing
void prvStopwatchSetSimulatedTime(uint32_t time)
{
	hwtc_count_simulated = time;
}


/******************************************************************************
 * Profiling results on 80 MHz Arm Cortex-M4 (STM32L475)
 *
 * Clock cycles needed for one measurement (xDfmStopwatchBegin() +
 * xDfmStopwatchEnd()) in the normal case (no alert or new high watermark).
 * - No Optimizations (-O0): 108 cycles
 * - Basic Optimizations (-O1): 65 cycles
 * - Full Optimizations (-O3): 31 cycles
 *
 * This means, with 1000 measurements per second on this 80 MHz device, the
 * execution time overhead would be 0,04%-0.14%.
 *****************************************************************************/

void prvTestOverhead(void)
{
    int counter = 0;
    dfmStopwatch_t* my_stopwatch;

    uint32_t starttime1;
    uint32_t endtime1;
    uint32_t max1 = 0;
    uint32_t starttime2;
    uint32_t endtime2;
    uint32_t max2 = 0;
    uint32_t dur;

    my_stopwatch = xDfmStopwatchCreate("MyStopwatch1", 10);
    if (my_stopwatch == (void*)0)
    {
    	printf("ERROR, my_stopwatch == NULL\n");
    	return;
    }

	starttime1 = TRC_HWTC_COUNT;
	vDfmStopwatchBegin(my_stopwatch);
	endtime1 = TRC_HWTC_COUNT;

	starttime2 = TRC_HWTC_COUNT;
	vDfmStopwatchEnd(my_stopwatch);
	endtime2 = TRC_HWTC_COUNT;

	dur = endtime1 - starttime1;
	printf("Begin: %u", (unsigned int)dur);

	if (dur > max1)
	{
		printf(" - new high watermark.\n");
		max1 = dur;
	}
	else
	{
		printf("\n");
	}

	dur = endtime2 - starttime2;
	printf("End: %u", (unsigned int)dur);
	if (dur > max2)
	{
		printf(" - new high watermark.\n");
		max2 = dur;
	}
	else
	{
		printf("\n");
	}
}

#endif

