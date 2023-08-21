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
 * @brief DFM serial port cloud port config
 */

#ifndef DFM_CLOUD_PORT_CONFIG_H
#define DFM_CLOUD_PORT_CONFIG_H

#if CONFIG_PERCEPIO_DFM_CFG_CLOUDPORT_SERIAL == 1

/**
 * @brief How to output the alert data over the serial port.
 */
#define DFM_PRINT_ALERT_DATA printk

/**
 * @brief Maximum size of the MQTT topic.
 */
#define DFM_CFG_CLOUD_PORT_MAX_TOPIC_SIZE (256U)

/**
 * @brief Intended to ensure exclusive access to the serial port before
 * outputting the alert data, to avoid that other output (random printf
 * calls) occur in between the "[[" and "]]" markers.
 * This can be implemented by disabling the kernel scheduler.
 * Note: This is mainly needed if calling xDfmAlertBegin directly from
 * task context. It is typically not needed on alerts from hard faults or
 * DFM_TRAP calls, as the prints then occur from an exception handler.
 */
#define DFM_CFG_LOCK_SERIAL()

/**
 * @brief Intended to release the serial port after having written a
 * block of alert data. If the kernel scheduler was disabled by the above
 * macro, it should be resumed here.
 */
#define DFM_CFG_UNLOCK_SERIAL()

#endif

#endif //UNITTEST_DFMCLOUDPORTCONFIG_H
