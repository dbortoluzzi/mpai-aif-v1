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

bool _set_aim_data_mic(MPAI_Component_AIM_t* aim);
bool _set_aim_produce_sensors(MPAI_Component_AIM_t* aim);
bool _set_aim_data_motion(MPAI_Component_AIM_t* aim);
bool _set_aim_rehabilitation(MPAI_Component_AIM_t* aim);
bool _set_aim_temp_limit(MPAI_Component_AIM_t* aim);

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
	message_store_channel_list[mpai_message_store_channel_count++] = sensors_data_channel;
    MIC_BUFFER_DATA_CHANNEL = MPAI_MessageStore_new_channel();
	channel_map_element_t mic_buffer_data_channel = {._channel_name = MPAI_LIBS_CAE_REV_MIC_BUFFER_DATA_CHANNEL_NAME, ._channel = MIC_BUFFER_DATA_CHANNEL};
	message_store_channel_list[mpai_message_store_channel_count++] = mic_buffer_data_channel;
    MIC_PEAK_DATA_CHANNEL = MPAI_MessageStore_new_channel();
	channel_map_element_t mic_peak_data_channel = {._channel_name = MPAI_LIBS_CAE_REV_MIC_PEAK_DATA_CHANNEL_NAME, ._channel = MIC_PEAK_DATA_CHANNEL};
	message_store_channel_list[mpai_message_store_channel_count++] = mic_peak_data_channel;
	MOTION_DATA_CHANNEL = MPAI_MessageStore_new_channel();
	channel_map_element_t motion_data_channel = {._channel_name = MPAI_LIBS_CAE_REV_MOTION_DATA_CHANNEL_NAME, ._channel = MOTION_DATA_CHANNEL};
	message_store_channel_list[mpai_message_store_channel_count++] = motion_data_channel;

	// add aims to list with related callback
	aim_initialization_cb_t aim_data_mic_init_cb = {._aim_name = MPAI_LIBS_CAE_REV_AIM_DATA_MIC_NAME, ._aim = aim_data_mic, ._post_cb = _set_aim_data_mic, ._subscriber = data_mic_aim_subscriber, ._start = data_mic_aim_start, ._stop = data_mic_aim_stop, ._resume = data_mic_aim_resume, ._pause = data_mic_aim_pause};
	MPAI_AIM_List[mpai_controller_aim_count++] = aim_data_mic_init_cb;
	aim_initialization_cb_t aim_data_sensors_init_cb = {._aim_name = MPAI_LIBS_CAE_REV_AIM_SENSORS_NAME, ._aim = aim_produce_sensors, ._post_cb = _set_aim_produce_sensors, ._subscriber = sensors_aim_subscriber, ._start = sensors_aim_start, ._stop = sensors_aim_stop, ._resume = sensors_aim_resume, ._pause = sensors_aim_pause};
	MPAI_AIM_List[mpai_controller_aim_count++] = aim_data_sensors_init_cb;
	aim_initialization_cb_t aim_temp_limit_init_cb = {._aim_name = MPAI_LIBS_CAE_REV_AIM_TEMP_LIMIT_NAME, ._aim = aim_temp_limit, ._post_cb = _set_aim_temp_limit, ._subscriber = temp_limit_aim_subscriber, ._start = temp_limit_aim_start, ._stop = temp_limit_aim_stop, ._resume = temp_limit_aim_resume, ._pause = temp_limit_aim_pause};
	MPAI_AIM_List[mpai_controller_aim_count++] = aim_temp_limit_init_cb;
	aim_initialization_cb_t aim_motion_init_cb = {._aim_name = MPAI_LIBS_CAE_REV_AIM_MOTION_NAME, ._aim = aim_data_motion, ._post_cb = _set_aim_data_motion, ._subscriber = motion_aim_subscriber, ._start = motion_aim_start, ._stop = motion_aim_stop, ._resume = motion_aim_resume, ._pause = motion_aim_pause};
	MPAI_AIM_List[mpai_controller_aim_count++] = aim_motion_init_cb;
	aim_initialization_cb_t aim_rehabilitation_init_cb = {._aim_name = MPAI_LIBS_CAE_REV_AIM_REHABILITATION_NAME, ._aim = aim_rehabilitation, ._post_cb = _set_aim_rehabilitation, ._subscriber = rehabilitation_aim_subscriber, ._start = rehabilitation_aim_start, ._stop = rehabilitation_aim_stop, ._resume = rehabilitation_aim_resume, ._pause = rehabilitation_aim_pause};
	MPAI_AIM_List[mpai_controller_aim_count++] = aim_rehabilitation_init_cb;

	return AIW_CAE_REV;
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
	#ifdef CONFIG_MPAI_AIM_CONTROL_UNIT_SENSORS
		MPAI_AIM_Destructor(aim_produce_sensors);
	#endif
	#ifdef CONFIG_MPAI_AIM_VOLUME_PEAKS_ANALYSIS
		MPAI_AIM_Destructor(aim_data_mic);
	#endif

}

bool _set_aim_data_mic(MPAI_Component_AIM_t* aim)
{
	#ifndef CONFIG_MPAI_AIM_VOLUME_PEAKS_ANALYSIS
		return false;
	#else
		aim_data_mic = aim;
		return true;
	#endif
}
bool _set_aim_produce_sensors(MPAI_Component_AIM_t* aim)
{

	#ifndef CONFIG_MPAI_AIM_CONTROL_UNIT_SENSORS
		return false;
	#else
		aim_produce_sensors = aim;
		return true;
	#endif
}
bool _set_aim_data_motion(MPAI_Component_AIM_t* aim)
{
	#ifndef CONFIG_MPAI_AIM_MOTION_RECOGNITION_ANALYSIS
		return false;
	#else
		aim_data_motion = aim;
		MPAI_MessageStore_register(message_store_test_case_aiw, MPAI_AIM_Get_Subscriber(aim_data_motion), SENSORS_DATA_CHANNEL);
		return true;
	#endif
}
bool _set_aim_rehabilitation(MPAI_Component_AIM_t* aim)
{
	#ifndef CONFIG_MPAI_AIM_VALIDATION_MOVEMENT_WITH_AUDIO
		return false;
	#else
		aim_rehabilitation = aim;
		MPAI_MessageStore_register(message_store_test_case_aiw, MPAI_AIM_Get_Subscriber(aim_rehabilitation), MOTION_DATA_CHANNEL);
		MPAI_MessageStore_register(message_store_test_case_aiw, MPAI_AIM_Get_Subscriber(aim_rehabilitation), MIC_PEAK_DATA_CHANNEL);
		return true;
	#endif
}
bool _set_aim_temp_limit(MPAI_Component_AIM_t* aim)
{
	#ifndef CONFIG_MPAI_AIM_TEMP_LIMIT
		return false;
	#else
		aim_temp_limit = aim;
		return true;
	#endif
}
