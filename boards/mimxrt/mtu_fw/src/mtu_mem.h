/*
 * Copyright 2024 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _MTU_MEM_H_
#define _MTU_MEM_H_

#include "mtu_mem_nor_ops.h"
#include "mtu_mem_nor_device.h"
#include "mtu_mem_ram_ops.h"
#include "mtu_mem_ram_device.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define MTU_MEM_MAX_MAP_SIZE (512 * 1024 * 1024UL)

/*******************************************************************************
 * Variables
 ******************************************************************************/


/*******************************************************************************
 * API
 ******************************************************************************/

status_t mtu_memory_init(void);

status_t mtu_memory_get_info(void);

status_t mtu_memory_rwtest(uint8_t memType, uint32_t memStart, uint32_t memSize, uint32_t memPattern);

#endif /* _MTU_MEM_H_ */
