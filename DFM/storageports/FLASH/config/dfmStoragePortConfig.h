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
 * @brief DFM FLASH storage port config
 */

#ifndef DFM_STORAGE_PORT_CONFIG_H
#define DFM_STORAGE_PORT_CONFIG_H

/* How much space is needed to store the alert to flash.
 * This should be a multiple of the flash page size (= 2048 on STM32L475). */
#define DFM_CFG_FLASHSTORAGE_SIZE (3 * 2048)

#endif //UNITTEST_DFMCLOUDPORTCONFIG_H
