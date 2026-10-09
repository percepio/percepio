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
 * @brief DFM serial port cloud port config
 */

#ifndef DFM_CLOUD_PORT_CONFIG_H
#define DFM_CLOUD_PORT_CONFIG_H

/**
 * @brief What ITM port to use for writing the alert data (0-31). Default port 2.
 * Ports 0, 1 and 31 are sometimes used for other purposes.
 */
#define DFM_CFG_ITM_PORT 2

/**
 * @brief Maximum size of the topic string.
 */
#define DFM_CFG_CLOUD_PORT_MAX_TOPIC_SIZE (256U)

#endif
