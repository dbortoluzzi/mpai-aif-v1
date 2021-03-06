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
#include <string.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <logging/log.h>

#include <errno.h>
#include <sys/byteorder.h>
#include <aif_metadata_parser.h>

#ifdef CONFIG_APP_TEST_WRITE_TO_FLASH
	#include <flash_store.h>
#endif

#include <aiw_iot_rev.h>

#ifdef CONFIG_COAP_SERVER
	#include <coap_connect.h>
#endif
#ifdef CONFIG_MPAI_CONFIG_STORE	
	#include <config_store.h>
#endif

#define WHOAMI_REG 0x0F
#define WHOAMI_ALT_REG 0x4F

#define MPAI_AIF_AIM_MAX 10
#define MPAI_AIF_CHANNEL_MAX 10
#define MPAI_AIF_AIW_MAX 10

/* Data structure usefull to initialize an AIM*/
typedef struct _aim_initialization_cb_t{
    char* _aim_name;
    MPAI_Component_AIM_t* _aim; 
	module_t* _subscriber; 	 				// related AIM subscriber identifier
	module_t* _start;		 				// AIM's start function
	module_t* _stop;		 				// AIM's stop function
	module_t* _resume;		 				// AIM's resume function
	module_t* _pause;		 				// AIM's pause function
	subscriber_channel_t* _input_channels;	// AIM subscribes to these input channels
	int8_t _count_channels;					// number of AIM's input channels
} aim_initialization_cb_t;

/* Tuple of channel and related channel_name */
typedef struct _channel_map_element_t{
    char* _channel_name;
    subscriber_channel_t _channel;
} channel_map_element_t;

typedef struct _message_store_map_element_t{
    int _aiw_id;
    MPAI_AIM_MessageStore_t*  _message_store;
} message_store_map_element_t;

/* AIM initialization List */
extern aim_initialization_cb_t* MPAI_AIM_List[MPAI_AIF_AIM_MAX];
/* Channel List */
extern channel_map_element_t message_store_channel_list[MPAI_AIF_CHANNEL_MAX];
/* Message Store List */
extern message_store_map_element_t message_store_list[MPAI_AIF_AIW_MAX];
extern int mpai_controller_aim_count;
extern int mpai_message_store_channel_count;
extern int mpai_message_store_count;

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

/**
 * @brief Start specified MPAI AIW
 * 
 * @param name name of the AIW
 * @param AIW_ID AIW_ID generated
 * @return error_t 
 */
mpai_error_t MPAI_AIFU_AIW_Start(const char* name, int* AIW_ID);

/**
 * @brief Start specified MPAI AIW
 * 
 * @param name name of the AIW
 * @param AIW_ID AIW_ID generated
 * @return error_t 
 */
mpai_error_t MPAI_AIFU_AIW_Pause(int AIW_ID);

/**
 * @brief Resume specified MPAI AIW
 * 
 * @param AIW_ID 
 * @return mpai_error_t 
 */
mpai_error_t MPAI_AIFU_AIW_Resume(int AIW_ID);

/**
 * @brief Stop specified MPAI AIW
 * 
 * @param AIW_ID 
 * @return mpai_error_t 
 */
mpai_error_t MPAI_AIFU_AIW_Stop(int AIW_ID);

/**
 * @brief Get AIM Status of an AIW
 * 
 * @param AIW_ID 
 * @param name 
 * @param status 
 * @return error_t 
 */
mpai_error_t MPAI_AIFU_AIM_GetStatus(int AIW_ID, const char* name, int* status);

/**
 * @brief Starts an AIW by name
 * 
 * @param name 
 * @return mpai_error_t 
 */
mpai_error_t MPAI_AIFM_AIM_Start(const char *name);

/**
 * @brief Stop an AIW by name
 * 
 * @param name 
 * @return mpai_error_t 
 */
mpai_error_t MPAI_AIFM_AIM_Stop(const char *name);

/**
 * @brief Pauses an AIW by name
 * 
 * @param name 
 * @return mpai_error_t 
 */
mpai_error_t MPAI_AIFM_AIM_Pause(const char *name);

/**
 * @brief Resumes an AIW by name
 * 
 * @param name 
 * @return mpai_error_t 
 */
mpai_error_t MPAI_AIFM_AIM_Resume(const char *name);

/**
 * @brief Initialize and start an AIM reading a configuration struct
 * 
 * @param aiw_id 
 * @param aim_init 
 * @return mpai_error_t 
 */
mpai_error_t MPAI_Controller_Start_Loading_AIM_From_Init_Config(int aiw_id, aim_initialization_cb_t* aim_init);

#if defined(CONFIG_MPAI_CONFIG_STORE)
/**
 * @brief Start an AIW, loading configurations from MPAI Store
 * 
 * @param name 
 * @return mpai_error_t 
 */
mpai_error_t MPAI_Controller_Start_Loading_AIW_From_MPAI_Store(const char *name, int aiw_id);
#endif

/**
 * @brief Retrieve AIM Initialization config of an AIM
 * 
 * @param name 
 * @return aim_initialization_cb_t* 
 */
aim_initialization_cb_t *MPAI_Controller_Find_AIM_Init_Config(const char *name);


#endif