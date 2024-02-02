/*
 * Percepio DFM v2.1.0
 * Copyright 2023 Percepio AB
 * www.percepio.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief DFM AWS Cloud port config
 */

#ifndef DFM_CLOUD_PORT_CONFIG_H
#define DFM_CLOUD_PORT_CONFIG_H

/**
 * @brief The prefix and rule to use in the MQTT message.
 */
#define DFM_CFG_MQTT_PREFIX "$aws/rules/DevAlertRule/"
/**
 * @brief Delay for retrying if establish TLS sessions fails.
 */
#define RETRY_BACKOFF_MS			   (500U)

/**
 * @brief Timeout for keeping the MQTT connection alive.
 */
#define KEEP_ALIVE_TIMEOUT_SECONDS	   (1800U) /* KEEP_ALIVE = 0 means disabled */

/**
 * @brief Timeout limit for receiving MQTT ack.
 */
#define CONNACK_RECV_TIMEOUT_MS		   (1000U)

/**
 * @brief Timeout for send and receive for TLS.
 */
#define TRANSPORT_SEND_RECV_TIMEOUT_MS (500U)

/**
 * @brief Maximum size of the MQTT topic.
 */
#define DFM_CFG_CLOUD_PORT_MAX_TOPIC_SIZE (1024U)

/**
 * @brief Size of the network buffer for MQTT packets.
 */
#define NETWORK_BUFFER_SIZE (1024U)

#endif //UNITTEST_DFMCLOUDPORTCONFIG_H
