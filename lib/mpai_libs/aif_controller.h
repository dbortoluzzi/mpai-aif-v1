/*
 * @file
 * @brief Headers of a MPAI AIF Controller, according to the specs V1
 * 
 * Copyright (c) 2022 University of Turin, Daniele Bortoluzzi <danieleb88@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MPAI_LIBS_AIF_CONTROLLER_H
#define MPAI_LIBS_AIF_CONTROLLER_H

#include <drivers/gpio.h>
#include <drivers/led.h>
#include <drivers/i2c.h>
#include <drivers/spi.h>
#include <drivers/sensor.h>
#include <usb/usb_device.h>
#include <drivers/uart.h>
#include <stdio.h>

#include <core_aim.h>
#include <sensors_aim.h>
#include <temp_limit_aim.h>
#include <data_mic_aim.h>
#include <motion_aim.h>
#include <rehabilitation_aim.h>
#include <message_store.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <logging/log.h>

#include <errno.h>
#include <sys/byteorder.h>

#ifdef CONFIG_APP_TEST_WRITE_TO_FLASH
	#include <flash_store.h>
#endif
#include <parson.h>

#include <test_use_case_aiw.h>

#ifdef CONFIG_COAP_SERVER
	#include <coap_connect.h>
#endif
#ifdef CONFIG_MPAI_CONFIG_STORE	
	#include <config_store.h>
#endif

#define WHOAMI_REG 0x0F
#define WHOAMI_ALT_REG 0x4F

/* AIMs to configured */
extern MPAI_Component_AIM_t* aim_produce_sensors;
extern MPAI_Component_AIM_t* aim_temp_limit;
extern MPAI_Component_AIM_t* aim_data_mic;
extern MPAI_Component_AIM_t* aim_data_motion;
extern MPAI_Component_AIM_t* aim_rehabilitation;

/**
 * @brief Initialize the MPAI Libs AIF Controller
 * 
 */
mpai_error_t MPAI_AIFU_Controller_Initialize();

/**
 * @brief Destroy the MPAI Libs AIF Controller
 * 
 * @return mpai_error_t 
 */
mpai_error_t MPAI_AIFU_Controller_Destroy();

#endif