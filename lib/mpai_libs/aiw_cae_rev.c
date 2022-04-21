/*
 * @file
 * @brief Implementation of the MPAI AIW CAE-REV (Rehabilitation Exercises Validation)
 * 
 * Copyright (c) 2022 University of Turin, Daniele Bortoluzzi <danieleb88@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include "aiw_cae_rev.h"

LOG_MODULE_REGISTER(MPAI_LIBS_AIW_CAE_REV, LOG_LEVEL_INF);

/* AIW global message store */
MPAI_AIM_MessageStore_t* message_store_test_case_aiw;

/* AIW global channels used by message store */
subscriber_channel_t SENSORS_DATA_CHANNEL;
subscriber_channel_t MIC_BUFFER_DATA_CHANNEL;
subscriber_channel_t MIC_PEAK_DATA_CHANNEL;
subscriber_channel_t MOTION_DATA_CHANNEL;

/* AIMs to be configured */
MPAI_Component_AIM_t* aim_data_mic = NULL;
MPAI_Component_AIM_t* aim_produce_sensors = NULL;
MPAI_Component_AIM_t* aim_temp_limit = NULL;
MPAI_Component_AIM_t* aim_data_motion = NULL;
MPAI_Component_AIM_t* aim_rehabilitation = NULL;

/* AIM initialization List */
aim_initialization_cb_t MPAI_AIM_List[MPAI_LIBS_CAE_REV_AIM_MAX] = {};
channel_map_element_t message_store_channel_list[MPAI_LIBS_CAE_REV_CHANNEL_MAX] = {};
void _set_aim_data_mic(MPAI_Component_AIM_t* aim);
void _set_aim_produce_sensors(MPAI_Component_AIM_t* aim);
void _set_aim_data_motion(MPAI_Component_AIM_t* aim);
void _set_aim_rehabilitation(MPAI_Component_AIM_t* aim);
void _set_aim_temp_limit(MPAI_Component_AIM_t* aim);

/************* PRIVATE HEADER *************/
channel_map_element_t _linear_search_channel(const char* name);
aim_initialization_cb_t _linear_search_aim(const char* name);

mpai_error_t init_data_mic_aim();
mpai_error_t init_sensors_aim();
mpai_error_t init_temp_limit_aim();
mpai_error_t init_motion_aim();
mpai_error_t init_rehabilitation_aim();

/************* PUBLIC HEADER *************/
int MPAI_AIW_CAE_REV_Init() 
{
    // create message store for the AIW
    message_store_test_case_aiw = MPAI_MessageStore_Creator(AIW_CAE_REV, MPAI_LIBS_CAE_REV_AIW_NAME, sizeof(mpai_parser_t));
    // link global message store to single message stores of the AIMs 
    message_store_data_mic_aim = message_store_test_case_aiw;
    message_store_sensors_aim = message_store_test_case_aiw;
    message_store_temp_limit_aim = message_store_test_case_aiw;
    message_store_motion_aim = message_store_test_case_aiw;
    message_store_rehabilitation_aim = message_store_test_case_aiw;

    // create channels
    SENSORS_DATA_CHANNEL = MPAI_MessageStore_new_channel();
	channel_map_element_t sensors_data_channel = {._channel_name = MPAI_LIBS_CAE_REV_SENSORS_DATA_CHANNEL_NAME, ._channel = SENSORS_DATA_CHANNEL};
	message_store_channel_list[channel_count++] = sensors_data_channel;
    MIC_BUFFER_DATA_CHANNEL = MPAI_MessageStore_new_channel();
	channel_map_element_t mic_buffer_data_channel = {._channel_name = MPAI_LIBS_CAE_REV_MIC_BUFFER_DATA_CHANNEL_NAME, ._channel = MIC_BUFFER_DATA_CHANNEL};
	message_store_channel_list[channel_count++] = mic_buffer_data_channel;
    MIC_PEAK_DATA_CHANNEL = MPAI_MessageStore_new_channel();
	channel_map_element_t mic_peak_data_channel = {._channel_name = MPAI_LIBS_CAE_REV_MIC_PEAK_DATA_CHANNEL_NAME, ._channel = MIC_PEAK_DATA_CHANNEL};
	message_store_channel_list[channel_count++] = mic_peak_data_channel;
	MOTION_DATA_CHANNEL = MPAI_MessageStore_new_channel();
	channel_map_element_t motion_data_channel = {._channel_name = MPAI_LIBS_CAE_REV_MOTION_DATA_CHANNEL_NAME, ._channel = MOTION_DATA_CHANNEL};
	message_store_channel_list[channel_count++] = motion_data_channel;

	// add aims to list with related callback
	aim_initialization_cb_t aim_data_mic_init_cb = {._aim_name = MPAI_LIBS_CAE_REV_AIM_DATA_MIC_NAME, ._aim = aim_data_mic, ._post_cb = _set_aim_data_mic, ._subscriber = data_mic_aim_subscriber, ._start = data_mic_aim_start, ._stop = data_mic_aim_stop, ._resume = data_mic_aim_resume, ._pause = data_mic_aim_pause};
	MPAI_AIM_List[aim_count++] = aim_data_mic_init_cb;
	aim_initialization_cb_t aim_data_sensors_init_cb = {._aim_name = MPAI_LIBS_CAE_REV_AIM_SENSORS_NAME, ._aim = aim_produce_sensors, ._post_cb = _set_aim_produce_sensors, ._subscriber = sensors_aim_subscriber, ._start = sensors_aim_start, ._stop = sensors_aim_stop, ._resume = sensors_aim_resume, ._pause = sensors_aim_pause};
	MPAI_AIM_List[aim_count++] = aim_data_sensors_init_cb;
	aim_initialization_cb_t aim_temp_limit_init_cb = {._aim_name = MPAI_LIBS_CAE_REV_AIM_TEMP_LIMIT_NAME, ._aim = aim_temp_limit, ._post_cb = _set_aim_temp_limit, ._subscriber = temp_limit_aim_subscriber, ._start = temp_limit_aim_start, ._stop = temp_limit_aim_stop, ._resume = temp_limit_aim_resume, ._pause = temp_limit_aim_pause};
	MPAI_AIM_List[aim_count++] = aim_temp_limit_init_cb;
	aim_initialization_cb_t aim_motion_init_cb = {._aim_name = MPAI_LIBS_CAE_REV_AIM_MOTION_NAME, ._aim = aim_data_motion, ._post_cb = _set_aim_data_motion, ._subscriber = motion_aim_subscriber, ._start = motion_aim_start, ._stop = motion_aim_stop, ._resume = motion_aim_resume, ._pause = motion_aim_pause};
	MPAI_AIM_List[aim_count++] = aim_motion_init_cb;
	aim_initialization_cb_t aim_rehabilitation_init_cb = {._aim_name = MPAI_LIBS_CAE_REV_AIM_REHABILITATION_NAME, ._aim = aim_rehabilitation, ._post_cb = _set_aim_rehabilitation, ._subscriber = rehabilitation_aim_subscriber, ._start = rehabilitation_aim_start, ._stop = rehabilitation_aim_stop, ._resume = rehabilitation_aim_resume, ._pause = rehabilitation_aim_pause};
	MPAI_AIM_List[aim_count++] = aim_rehabilitation_init_cb;

	return AIW_CAE_REV;
}

void MPAI_AIW_CAE_REV_Start()
{
	#if defined(CONFIG_MPAI_CONFIG_STORE) && defined (CONFIG_MPAI_CONFIG_STORE_USES_COAP)
		char* aiw_result = MPAI_Config_Store_Get_AIW(MPAI_LIBS_CAE_REV_AIW_NAME);
		// printk("AIW RESULT: \n");
		// for ( size_t i = 0; i < strlen(aiw_result); i++ )
		// {
		// 	printk("%c", (char)aiw_result[i]);
		// 	k_sleep(K_MSEC(5));
		// }
		// printk("\n");

		JSON_Value* json_aiw = json_parse_string(aiw_result);
		char* aiw_name = json_object_get_string(json_object(json_aiw), "title");
		LOG_INF("Initializing AIW with title \"%s\"...", log_strdup(aiw_name));

		JSON_Array* json_aiw_subaims = json_object_get_array(json_object(json_aiw), "SubAIMs");
		for (size_t i = 0; i < json_array_get_count(json_aiw_subaims); i++) {
			JSON_Object* aiw_subaim = json_array_get_object(json_aiw_subaims, i);
			const char* aim_name = json_object_dotget_string(aiw_subaim, "Identifier.Specification.AIM");

			aim_initialization_cb_t aim_init_cb = _linear_search_aim(aim_name);
			if (aim_init_cb._aim_name != NULL) {
				LOG_INF("AIM %s found for AIW %s, now initializing...", log_strdup(aim_name), log_strdup(aiw_name));

				char* aim_result = MPAI_Config_Store_Get_AIM(aim_name);
				if (aim_result != NULL) {
					LOG_DBG("Calling AIM %s: success", log_strdup(aim_name));
					mpai_error_t err_aim = MPAI_AIFU_AIM_Start(AIW_CAE_REV, aim_init_cb);
					if (err_aim.code != MPAI_AIF_OK)
					{
						    LOG_ERR("Stop initialization");
							while (1) {};
					}
				}
				k_free(aim_result);
			} else 
			{
				LOG_ERR("AIM %s not found", log_strdup(aim_name));
			}
			k_sleep(K_MSEC(50));
    	}

		k_free(aiw_result);
	#endif

	#ifdef CONFIG_MPAI_AIM_CONTROL_UNIT_SENSORS_PERIODIC
		/* start periodic timer to switch status */
		k_timer_start(&aim_timer, K_SECONDS(5), K_SECONDS(5));		
	#endif
}

void MPAI_AIW_CAE_REV_Stop() 
{
	#ifdef CONFIG_MPAI_AIM_VALIDATION_MOVEMENT_WITH_AUDIO
		MPAI_AIM_Stop(aim_rehabilitation);
	#endif
	#ifdef CONFIG_MPAI_AIM_MOTION_RECOGNITION_ANALYSIS
		MPAI_AIM_Stop(aim_data_motion);
	#endif
	#ifdef CONFIG_MPAI_AIM_TEMP_LIMIT
		MPAI_AIM_Stop(aim_temp_limit);
	#endif
	#ifdef CONFIG_MPAI_AIM_CONTROL_UNIT_SENSORS
		MPAI_AIM_Stop(aim_produce_sensors);
	#endif
	#ifdef CONFIG_MPAI_AIM_VOLUME_PEAKS_ANALYSIS
		MPAI_AIM_Stop(aim_data_mic);
	#endif

	memset ( MPAI_AIM_List, 0, MPAI_LIBS_CAE_REV_AIM_MAX*sizeof(MPAI_Component_AIM_t*) ) ;
}

void MPAI_AIW_CAE_REV_Resume()
{
	#ifdef CONFIG_MPAI_AIM_CONTROL_UNIT_SENSORS
		MPAI_AIM_Resume(aim_produce_sensors);
	#endif
	#ifdef CONFIG_MPAI_AIM_VOLUME_PEAKS_ANALYSIS
		MPAI_AIM_Resume(aim_data_mic);
	#endif
	#ifdef CONFIG_MPAI_AIM_TEMP_LIMIT
		MPAI_AIM_Resume(aim_temp_limit);
	#endif
	#ifdef CONFIG_MPAI_AIM_VALIDATION_MOVEMENT_WITH_AUDIO
		MPAI_AIM_Resume(aim_rehabilitation);
	#endif
	#ifdef CONFIG_MPAI_AIM_MOTION_RECOGNITION_ANALYSIS
		MPAI_AIM_Resume(aim_data_motion);
	#endif
}

void MPAI_AIW_CAE_REV_Pause()
{
	#ifdef CONFIG_MPAI_AIM_VALIDATION_MOVEMENT_WITH_AUDIO
		MPAI_AIM_Pause(aim_rehabilitation);
	#endif
	#ifdef CONFIG_MPAI_AIM_MOTION_RECOGNITION_ANALYSIS
		MPAI_AIM_Pause(aim_data_motion);
	#endif
	#ifdef CONFIG_MPAI_AIM_TEMP_LIMIT
		MPAI_AIM_Pause(aim_temp_limit);
	#endif
	#ifdef CONFIG_MPAI_AIM_CONTROL_UNIT_SENSORS
		MPAI_AIM_Pause(aim_produce_sensors);
	#endif
	#ifdef CONFIG_MPAI_AIM_VOLUME_PEAKS_ANALYSIS
		MPAI_AIM_Pause(aim_data_mic);
	#endif
}

void DESTROY_MPAI_AIW_CAE_REV() 
{
	MPAI_AIW_CAE_REV_Stop();

	k_sleep(K_SECONDS(2));

	MPAI_MessageStore_Destructor(message_store_test_case_aiw);

	#ifdef CONFIG_MPAI_AIM_VALIDATION_MOVEMENT_WITH_AUDIO
		MPAI_AIM_Destructor(aim_rehabilitation);
	#endif
	#ifdef CONFIG_MPAI_AIM_MOTION_RECOGNITION_ANALYSIS
		MPAI_AIM_Destructor(aim_data_motion);
	#endif
	#ifdef CONFIG_MPAI_AIM_TEMP_LIMIT
		MPAI_AIM_Destructor(aim_temp_limit);
	#endif
	MPAI_AIM_Destructor(aim_produce_sensors);
	#ifdef CONFIG_MPAI_AIM_VOLUME_PEAKS_ANALYSIS
		MPAI_AIM_Destructor(aim_data_mic);
	#endif

	memset ( MPAI_AIM_List, 0, MPAI_LIBS_CAE_REV_AIM_MAX*sizeof(MPAI_Component_AIM_t*) ) ;
}

aim_initialization_cb_t _linear_search_aim(const char* name)
{
	for (size_t i = 0; i < aim_count; i++)
	{
		// verify aim name
		if (strcmp(MPAI_AIM_Get_Component(MPAI_AIM_List[i]._aim)->name, name) == 0 || strcmp(MPAI_AIM_List[i]._aim_name, name) == 0)
		{
			return MPAI_AIM_List[i];
		}
	}
	aim_initialization_cb_t empty = {};
	return empty;
}

channel_map_element_t _linear_search_channel(const char* name)
{
	for (size_t i = 0; i < channel_count; i++)
	{
		// verify aim name
		if (strcmp(message_store_channel_list[i]._channel_name, name) == 0)
		{
			return message_store_channel_list[i];
		}
	}
	channel_map_element_t empty = {};
	return empty;
}

void _set_aim_data_mic(MPAI_Component_AIM_t* aim)
{
	aim_data_mic = aim;
}
void _set_aim_produce_sensors(MPAI_Component_AIM_t* aim)
{
	aim_produce_sensors = aim;
}
void _set_aim_data_motion(MPAI_Component_AIM_t* aim)
{
	aim_data_motion = aim;
	MPAI_MessageStore_register(message_store_test_case_aiw, MPAI_AIM_Get_Subscriber(aim_data_motion), SENSORS_DATA_CHANNEL);
}
void _set_aim_rehabilitation(MPAI_Component_AIM_t* aim)
{
	aim_rehabilitation = aim;
	MPAI_MessageStore_register(message_store_test_case_aiw, MPAI_AIM_Get_Subscriber(aim_rehabilitation), MOTION_DATA_CHANNEL);
	MPAI_MessageStore_register(message_store_test_case_aiw, MPAI_AIM_Get_Subscriber(aim_rehabilitation), MIC_PEAK_DATA_CHANNEL);
}
void _set_aim_temp_limit(MPAI_Component_AIM_t* aim)
{
	aim_temp_limit = aim;
}
