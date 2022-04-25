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

#ifdef CONFIG_MPAI_AIM_CONTROL_UNIT_SENSORS_PERIODIC

/******** START PERIODIC MODE ***********/
void aim_timer_switch_status(struct k_work *work)
{
	int* status = (int *)k_malloc(sizeof(int));
	mpai_error_t err = MPAI_AIFU_AIM_GetStatus(AIW_CAE_REV, MPAI_LIBS_CAE_REV_AIM_SENSORS_NAME, status);
	if (err.code == MPAI_AIF_OK)
	{
		if (MPAI_AIM_ALIVE == *status)
		{
			MPAI_AIM_Pause(aim_produce_sensors);
		}
		else
		{
			MPAI_AIM_Resume(aim_produce_sensors);
		}
	}
	k_free(status);
}

K_WORK_DEFINE(my_work, aim_timer_switch_status);

void aim_timer_handler(struct k_timer *dummy)
{
	k_work_submit(&my_work);
}

K_TIMER_DEFINE(aim_timer, aim_timer_handler, NULL);
/******** END PERIODIC MODE ***********/
#endif

/************* PUBLIC HEADER *************/
int MPAI_AIW_CAE_REV_Init() 
{
    // create message store for the AIW
    message_store_test_case_aiw = MPAI_MessageStore_Creator(AIW_CAE_REV, MPAI_LIBS_CAE_REV_AIW_NAME, sizeof(mpai_parser_t));
	message_store_map_element_t message_store_map_el_test_case_aiw = {._aiw_id = AIW_CAE_REV, ._message_store = message_store_test_case_aiw};
	message_store_list[mpai_message_store_count++] = message_store_map_el_test_case_aiw;
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
	aim_initialization_cb_t* aim_data_mic_init_cb = (aim_initialization_cb_t *) k_malloc(sizeof(aim_initialization_cb_t));
	aim_data_mic_init_cb->_aim_name = MPAI_LIBS_CAE_REV_AIM_DATA_MIC_NAME;
	aim_data_mic_init_cb->_subscriber = data_mic_aim_subscriber;
	aim_data_mic_init_cb->_start = data_mic_aim_start;
	aim_data_mic_init_cb->_stop = data_mic_aim_stop;
	aim_data_mic_init_cb->_resume = data_mic_aim_resume;
	aim_data_mic_init_cb->_pause = data_mic_aim_pause;
	aim_data_mic_init_cb->_input_channels = NULL;
	aim_data_mic_init_cb->_count_channels = 0;
	MPAI_AIM_List[mpai_controller_aim_count++] = aim_data_mic_init_cb;

	aim_initialization_cb_t* aim_data_sensors_init_cb = (aim_initialization_cb_t *) k_malloc(sizeof(aim_initialization_cb_t));
	aim_data_sensors_init_cb->_aim_name = MPAI_LIBS_CAE_REV_AIM_SENSORS_NAME;
	aim_data_sensors_init_cb->_subscriber = sensors_aim_subscriber;
	aim_data_sensors_init_cb->_start = sensors_aim_start;
	aim_data_sensors_init_cb->_stop = sensors_aim_stop;
	aim_data_sensors_init_cb->_resume = sensors_aim_resume;
	aim_data_sensors_init_cb->_pause = sensors_aim_pause;
	aim_data_sensors_init_cb->_input_channels = NULL;
	aim_data_sensors_init_cb->_count_channels = 0;
	MPAI_AIM_List[mpai_controller_aim_count++] = aim_data_sensors_init_cb;

	aim_initialization_cb_t* aim_temp_limit_init_cb = (aim_initialization_cb_t *) k_malloc(sizeof(aim_initialization_cb_t));
	aim_temp_limit_init_cb->_aim_name = MPAI_LIBS_CAE_REV_AIM_TEMP_LIMIT_NAME;
	aim_temp_limit_init_cb->_subscriber = temp_limit_aim_subscriber;
	aim_temp_limit_init_cb->_start = temp_limit_aim_start;
	aim_temp_limit_init_cb->_stop = temp_limit_aim_stop;
	aim_temp_limit_init_cb->_resume = temp_limit_aim_resume;
	aim_temp_limit_init_cb->_pause = temp_limit_aim_pause;
	aim_temp_limit_init_cb->_input_channels = NULL;
	aim_temp_limit_init_cb->_count_channels = 0;
	MPAI_AIM_List[mpai_controller_aim_count++] = aim_temp_limit_init_cb;

	aim_initialization_cb_t* aim_motion_init_cb = (aim_initialization_cb_t *) k_malloc(sizeof(aim_initialization_cb_t));
	aim_motion_init_cb->_aim_name = MPAI_LIBS_CAE_REV_AIM_MOTION_NAME;
	aim_motion_init_cb->_subscriber = motion_aim_subscriber;
	aim_motion_init_cb->_start = motion_aim_start;
	aim_motion_init_cb->_stop = motion_aim_stop;
	aim_motion_init_cb->_resume = motion_aim_resume;
	aim_motion_init_cb->_pause = motion_aim_pause;
	aim_motion_init_cb->_input_channels = NULL;
	aim_motion_init_cb->_count_channels = 0;
	MPAI_AIM_List[mpai_controller_aim_count++] = aim_motion_init_cb;

	aim_initialization_cb_t* aim_rehabilitation_init_cb = (aim_initialization_cb_t *) k_malloc(sizeof(aim_initialization_cb_t));
	aim_rehabilitation_init_cb->_aim_name = MPAI_LIBS_CAE_REV_AIM_REHABILITATION_NAME;
	aim_rehabilitation_init_cb->_subscriber = rehabilitation_aim_subscriber;
	aim_rehabilitation_init_cb->_start = rehabilitation_aim_start;
	aim_rehabilitation_init_cb->_stop = rehabilitation_aim_stop;
	aim_rehabilitation_init_cb->_resume = rehabilitation_aim_resume;
	aim_rehabilitation_init_cb->_pause = rehabilitation_aim_pause;
	aim_rehabilitation_init_cb->_input_channels = NULL;
	aim_rehabilitation_init_cb->_count_channels = 0;
	MPAI_AIM_List[mpai_controller_aim_count++] = aim_rehabilitation_init_cb;

	#ifdef CONFIG_MPAI_AIM_CONTROL_UNIT_SENSORS_PERIODIC
			/* start periodic timer to switch status */
			k_timer_start(&aim_timer, K_SECONDS(5), K_SECONDS(5));
	#endif

	return AIW_CAE_REV;
}

void MPAI_AIW_CAE_REV_Stop() 
{
	#ifdef CONFIG_MPAI_AIM_VALIDATION_MOVEMENT_WITH_AUDIO
		MPAI_AIFM_AIM_Stop(MPAI_LIBS_CAE_REV_AIM_REHABILITATION_NAME);
	#endif
	#ifdef CONFIG_MPAI_AIM_MOTION_RECOGNITION_ANALYSIS
		MPAI_AIFM_AIM_Stop(MPAI_LIBS_CAE_REV_AIM_MOTION_NAME);
	#endif
	#ifdef CONFIG_MPAI_AIM_TEMP_LIMIT
		MPAI_AIFM_AIM_Stop(MPAI_LIBS_CAE_REV_AIM_TEMP_LIMIT_NAME);
	#endif
	#ifdef CONFIG_MPAI_AIM_CONTROL_UNIT_SENSORS
		MPAI_AIFM_AIM_Stop(MPAI_LIBS_CAE_REV_AIM_SENSORS_NAME);
	#endif
	#ifdef CONFIG_MPAI_AIM_VOLUME_PEAKS_ANALYSIS
		MPAI_AIFM_AIM_Stop(MPAI_LIBS_CAE_REV_AIM_DATA_MIC_NAME);
	#endif
}

void MPAI_AIW_CAE_REV_Resume()
{
	#ifdef CONFIG_MPAI_AIM_CONTROL_UNIT_SENSORS
		MPAI_AIFM_AIM_Resume(MPAI_LIBS_CAE_REV_AIM_SENSORS_NAME);
	#endif
	#ifdef CONFIG_MPAI_AIM_VOLUME_PEAKS_ANALYSIS
		MPAI_AIFM_AIM_Resume(MPAI_LIBS_CAE_REV_AIM_DATA_MIC_NAME);
	#endif
	#ifdef CONFIG_MPAI_AIM_TEMP_LIMIT
		MPAI_AIFM_AIM_Resume(MPAI_LIBS_CAE_REV_AIM_TEMP_LIMIT_NAME);
	#endif
	#ifdef CONFIG_MPAI_AIM_VALIDATION_MOVEMENT_WITH_AUDIO
		MPAI_AIFM_AIM_Resume(MPAI_LIBS_CAE_REV_AIM_REHABILITATION_NAME);
	#endif
	#ifdef CONFIG_MPAI_AIM_MOTION_RECOGNITION_ANALYSIS
		MPAI_AIFM_AIM_Resume(MPAI_LIBS_CAE_REV_AIM_MOTION_NAME);
	#endif
}

void MPAI_AIW_CAE_REV_Pause()
{
	#ifdef CONFIG_MPAI_AIM_VALIDATION_MOVEMENT_WITH_AUDIO
		MPAI_AIFM_AIM_Pause(MPAI_LIBS_CAE_REV_AIM_REHABILITATION_NAME);
	#endif
	#ifdef CONFIG_MPAI_AIM_MOTION_RECOGNITION_ANALYSIS
		MPAI_AIFM_AIM_Pause(MPAI_LIBS_CAE_REV_AIM_MOTION_NAME);
	#endif
	#ifdef CONFIG_MPAI_AIM_TEMP_LIMIT
		MPAI_AIFM_AIM_Pause(MPAI_LIBS_CAE_REV_AIM_TEMP_LIMIT_NAME);
	#endif
	#ifdef CONFIG_MPAI_AIM_CONTROL_UNIT_SENSORS
		MPAI_AIFM_AIM_Pause(MPAI_LIBS_CAE_REV_AIM_SENSORS_NAME);
	#endif
	#ifdef CONFIG_MPAI_AIM_VOLUME_PEAKS_ANALYSIS
		MPAI_AIFM_AIM_Pause(MPAI_LIBS_CAE_REV_AIM_DATA_MIC_NAME);
	#endif
}

void DESTROY_MPAI_AIW_CAE_REV() 
{
	MPAI_AIW_CAE_REV_Stop();

	k_sleep(K_SECONDS(2));

	MPAI_MessageStore_Destructor(message_store_test_case_aiw);

	#ifdef CONFIG_MPAI_AIM_VALIDATION_MOVEMENT_WITH_AUDIO
		{
			aim_initialization_cb_t* aim_init = MPAI_AIFU_AIM_Find_AIM_Init_Config(MPAI_LIBS_CAE_REV_AIM_REHABILITATION_NAME);
			MPAI_AIM_Destructor(aim_init->_aim);
		}
	#endif
	#ifdef CONFIG_MPAI_AIM_MOTION_RECOGNITION_ANALYSIS
		{
			aim_initialization_cb_t* aim_init = MPAI_AIFU_AIM_Find_AIM_Init_Config(MPAI_LIBS_CAE_REV_AIM_MOTION_NAME);
			MPAI_AIM_Destructor(aim_init->_aim);
		}
	#endif
	#ifdef CONFIG_MPAI_AIM_TEMP_LIMIT
		{
			aim_initialization_cb_t* aim_init = MPAI_AIFU_AIM_Find_AIM_Init_Config(MPAI_LIBS_CAE_REV_AIM_TEMP_LIMIT_NAME);
			MPAI_AIM_Destructor(aim_init->_aim);
		}
	#endif
	#ifdef CONFIG_MPAI_AIM_CONTROL_UNIT_SENSORS
		{
			aim_initialization_cb_t* aim_init = MPAI_AIFU_AIM_Find_AIM_Init_Config(MPAI_LIBS_CAE_REV_AIM_SENSORS_NAME);
			MPAI_AIM_Destructor(aim_init->_aim);
		}
	#endif
	#ifdef CONFIG_MPAI_AIM_VOLUME_PEAKS_ANALYSIS
		{
			aim_initialization_cb_t* aim_init = MPAI_AIFU_AIM_Find_AIM_Init_Config(MPAI_LIBS_CAE_REV_AIM_DATA_MIC_NAME);
			MPAI_AIM_Destructor(aim_init->_aim);
		}
	#endif
}