/*
 * Copyright (c) 2022 University of Turin, Daniele Bortoluzzi <danieleb88@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "flash_store.h"

LOG_MODULE_REGISTER(MPAI_FLASH_STORE, LOG_LEVEL_INF);

/************* PRIVATE *************/

/************* PUBLIC **************/

struct device* init_flash()
{
	printf("\n" FLASH_NAME " SPI flash testing\n");
	printf("==========================\n");

	struct device* flash_dev = device_get_binding(FLASH_DEVICE);

	if (!flash_dev) {
		LOG_ERR("SPI flash driver %s was not found!\n",
		       FLASH_DEVICE);
		return NULL;
	}

	return flash_dev;
}

int erase_flash(const struct device* flash_dev)
{
	int rc = flash_erase(flash_dev, FLASH_TEST_REGION_OFFSET,
			 FLASH_SECTOR_SIZE);
	if (rc != 0) {
		LOG_ERR("Flash erase failed! %d\n", rc);
	} else {
		LOG_INF("Flash erase succeeded!\n");
	}
	return rc;
}

int write_flash(const struct device* flash_dev, size_t len, void* data)
{
	LOG_INF("Attempting to write %zu bytes\n", len);
	int rc = flash_write(flash_dev, FLASH_TEST_REGION_OFFSET, data, len);
	if (rc != 0) {
		LOG_ERR("Flash write failed! %d\n", rc);
		return rc;
	}
	return rc;
}

int read_flash(const struct device* flash_dev, size_t len, void* buf)
{
	memset(buf, 0, len);
	int rc = flash_read(flash_dev, FLASH_TEST_REGION_OFFSET, buf, len);
	if (rc != 0) {
		LOG_ERR("Flash read failed! %d\n", rc);
		return rc;
	}
	return rc;
}