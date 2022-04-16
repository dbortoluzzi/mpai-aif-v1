/*
 * Copyright (c) 2022 University of Turin, Daniele Bortoluzzi <danieleb88@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MPAI_FLASH_STORE_H
#define MPAI_FLASH_STORE_H

#include <drivers/flash.h>
#include <device.h>
#include <devicetree.h>
#include <stdio.h>
#include <string.h>
#include <logging/log.h>

#if (CONFIG_SPI_NOR - 0) ||				\
	DT_NODE_HAS_STATUS(DT_INST(0, jedec_spi_nor), okay)
#define FLASH_DEVICE DT_LABEL(DT_INST(0, jedec_spi_nor))
#define FLASH_NAME "JEDEC SPI-NOR"
#elif (CONFIG_NORDIC_QSPI_NOR - 0) || \
	DT_NODE_HAS_STATUS(DT_INST(0, nordic_qspi_nor), okay)
#define FLASH_DEVICE DT_LABEL(DT_INST(0, nordic_qspi_nor))
#define FLASH_NAME "JEDEC QSPI-NOR"
#elif DT_NODE_HAS_STATUS(DT_INST(0, st_stm32_qspi_nor), okay)
#define FLASH_DEVICE DT_LABEL(DT_INST(0, st_stm32_qspi_nor))
#define FLASH_NAME "JEDEC QSPI-NOR"
#else
#error Unsupported flash driver
#endif

#if defined(CONFIG_BOARD_ADAFRUIT_FEATHER_STM32F405)
#define FLASH_TEST_REGION_OFFSET 0xf000
#elif defined(CONFIG_BOARD_ARTY_A7_ARM_DESIGNSTART_M1) || \
	defined(CONFIG_BOARD_ARTY_A7_ARM_DESIGNSTART_M3)
/* The FPGA bitstream is stored in the lower 536 sectors of the flash. */
#define FLASH_TEST_REGION_OFFSET \
	DT_REG_SIZE(DT_NODE_BY_FIXED_PARTITION_LABEL(fpga_bitstream))
#else
#define FLASH_TEST_REGION_OFFSET 0xff000
#endif
#define FLASH_SECTOR_SIZE        4096

struct device* init_flash();

int erase_flash(const struct device* dev);

int write_flash(const struct device* dev, size_t len, void* data);

int read_flash(const struct device* dev, size_t len, void* buf);

#endif