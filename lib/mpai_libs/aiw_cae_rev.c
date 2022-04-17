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
// TODO: create a map of channel
subscriber_channel_t SENSORS_DATA_CHANNEL;
subscriber_channel_t MIC_BUFFER_DATA_CHANNEL;
subscriber_channel_t MIC_PEAK_DATA_CHANNEL;

/* AIMs to be configured */
MPAI_Component_AIM_t* aim_data_mic = NULL;
MPAI_Component_AIM_t* aim_produce_sensors = NULL;
MPAI_Component_AIM_t* aim_temp_limit = NULL;
MPAI_Component_AIM_t* aim_data_motion = NULL;
MPAI_Component_AIM_t* aim_rehabilitation = NULL;

/* AIM initialization List */
aim_initialization_cb_t MPAI_AIM_List[MPAI_LIBS_CAE_REV_AIM_COUNT] = {};

/************* PRIVATE HEADER *************/
aim_initialization_cb_t _linear_search_aim(char* name);

mpai_error_t* init_data_mic_aim();
mpai_error_t* init_sensors_aim();
mpai_error_t* init_temp_limit_aim();
mpai_error_t* init_motion_aim();
mpai_error_t* init_rehabilitation_aim();

/************* PUBLIC HEADER *************/
int INIT_Test_Use_Case_AIW() 
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
    MIC_BUFFER_DATA_CHANNEL = MPAI_MessageStore_new_channel();
    MIC_PEAK_DATA_CHANNEL = MPAI_MessageStore_new_channel();

	// add aims to list with related callback
	aim_initialization_cb_t aim_data_mic_init_cb = {._aim = aim_data_mic, ._init_cb = init_data_mic_aim};
	MPAI_AIM_List[0] = aim_data_mic_init_cb;
	aim_initialization_cb_t aim_data_sensors_init_cb = {._aim = aim_produce_sensors, ._init_cb = init_sensors_aim};
	MPAI_AIM_List[1] = aim_data_sensors_init_cb;
	aim_initialization_cb_t aim_temp_limit_init_cb = {._aim = aim_temp_limit, ._init_cb = init_temp_limit_aim};
	MPAI_AIM_List[2] = aim_temp_limit_init_cb;
	aim_initialization_cb_t aim_motion_init_cb = {._aim = aim_data_motion, ._init_cb = init_motion_aim};
	MPAI_AIM_List[3] = aim_motion_init_cb;
	aim_initialization_cb_t aim_rehabilitation_init_cb = {._aim = aim_rehabilitation, ._init_cb = init_rehabilitation_aim};
	MPAI_AIM_List[4] = aim_rehabilitation_init_cb;

	return AIW_CAE_REV;
}

void START_Test_Use_Case_AIW()
{
	// Start all initialization callbacks
	// TODO: initialize AIMs according with the JSONs retrieved from MPAI Config Store
	for (size_t i = 0; i < MPAI_LIBS_CAE_REV_AIM_COUNT; i++)
	{
		MPAI_AIM_List[i]._init_cb();
	}

	#ifdef CONFIG_MPAI_AIM_CONTROL_UNIT_SENSORS_PERIODIC
		/* start periodic timer to switch status */
		k_timer_start(&aim_timer, K_SECONDS(5), K_SECONDS(5));		
	#endif
}

void STOP_Test_Use_Case_AIW() 
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

	memset ( MPAI_AIM_List, 0, MPAI_LIBS_CAE_REV_AIM_COUNT*sizeof(MPAI_Component_AIM_t*) ) ;
}

void RESUME_Test_Use_Case_AIW()
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

void PAUSE_Test_Use_Case_AIW()
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

void DESTROY_Test_Use_Case_AIW() 
{
	STOP_Test_Use_Case_AIW();

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

	memset ( MPAI_AIM_List, 0, MPAI_LIBS_CAE_REV_AIM_COUNT*sizeof(MPAI_Component_AIM_t*) ) ;
}

aim_initialization_cb_t _linear_search_aim(char* name)
{
	for (size_t i = 0; i < MPAI_LIBS_CAE_REV_AIM_COUNT; i++)
	{
		// verify aim name
		if (strcmp(MPAI_AIM_Get_Component(MPAI_AIM_List[i]._aim)->name, name) == 0)
		{
			return MPAI_AIM_List[i];
		}
	}
	return;
}

mpai_error_t* init_data_mic_aim()
{
	#ifdef CONFIG_MPAI_AIM_VOLUME_PEAKS_ANALYSIS
		aim_data_mic = MPAI_AIM_Creator(MPAI_LIBS_CAE_REV_AIM_DATA_MIC_NAME, AIW_CAE_REV, data_mic_aim_subscriber, data_mic_aim_start, data_mic_aim_stop, data_mic_aim_resume, data_mic_aim_pause);
		mpai_error_t err_data_mic = MPAI_AIM_Start(aim_data_mic);	

		if (err_data_mic.code != MPAI_AIF_OK)
		{
			LOG_ERR("Error starting AIM %s: %s", log_strdup(MPAI_AIM_Get_Component(aim_data_mic)->name), log_strdup(MPAI_ERR_STR(err_data_mic.code)));
			return;
		} 
	#endif
}
mpai_error_t* init_sensors_aim()
{
	#ifdef CONFIG_MPAI_AIM_CONTROL_UNIT_SENSORS
		aim_produce_sensors = MPAI_AIM_Creator(MPAI_LIBS_CAE_REV_AIM_SENSORS_NAME, AIW_CAE_REV, sensors_aim_subscriber, sensors_aim_start, sensors_aim_stop, sensors_aim_resume, sensors_aim_pause);
		mpai_error_t err_sens_aim = MPAI_AIM_Start(aim_produce_sensors);

		if (err_sens_aim.code != MPAI_AIF_OK) 
		{
			LOG_ERR("Error starting AIM %s: %s", log_strdup(MPAI_AIM_Get_Component(aim_produce_sensors)->name), log_strdup(MPAI_ERR_STR(err_sens_aim.code)));
			return;
		}
	#endif
}
mpai_error_t* init_temp_limit_aim()
{
	#ifdef CONFIG_MPAI_AIM_TEMP_LIMIT
		aim_temp_limit = MPAI_AIM_Creator(MPAI_LIBS_CAE_REV_AIM_TEMP_LIMIT_NAME, AIW_CAE_REV, temp_limit_aim_subscriber, temp_limit_aim_start, temp_limit_aim_stop, temp_limit_aim_resume, temp_limit_aim_pause);
		MPAI_MessageStore_register(message_store_test_case_aiw, MPAI_AIM_Get_Subscriber(aim_temp_limit), SENSORS_DATA_CHANNEL);
		mpai_error_t err_temp_limit = MPAI_AIM_Start(aim_temp_limit);	

		if (err_temp_limit.code != MPAI_AIF_OK)
		{
			LOG_ERR("Error starting AIM %s: %s", log_strdup(MPAI_AIM_Get_Component(aim_temp_limit)->name), log_strdup(MPAI_ERR_STR(err_temp_limit.code)));
			return;
		} 
	#endif
}
mpai_error_t* init_motion_aim()
{
	#ifdef CONFIG_MPAI_AIM_MOTION_RECOGNITION_ANALYSIS
		aim_data_motion = MPAI_AIM_Creator(MPAI_LIBS_CAE_REV_AIM_MOTION_NAME, AIW_CAE_REV, motion_aim_subscriber, motion_aim_start, motion_aim_stop, motion_aim_resume, motion_aim_pause);
		MPAI_MessageStore_register(message_store_test_case_aiw, MPAI_AIM_Get_Subscriber(aim_data_motion), SENSORS_DATA_CHANNEL);
		mpai_error_t err_motion = MPAI_AIM_Start(aim_data_motion);	

		if (err_motion.code != MPAI_AIF_OK)
		{
			LOG_ERR("Error starting AIM %s: %s", log_strdup(MPAI_AIM_Get_Component(aim_data_motion)->name), log_strdup(MPAI_ERR_STR(err_motion.code)));
			return;
		} 
	#endif
}
mpai_error_t* init_rehabilitation_aim()
{

	#ifdef CONFIG_MPAI_AIM_VALIDATION_MOVEMENT_WITH_AUDIO
		aim_rehabilitation = MPAI_AIM_Creator(MPAI_LIBS_CAE_REV_AIM_REHABILITATION_NAME, AIW_CAE_REV, rehabilitation_aim_subscriber, rehabilitation_aim_start, rehabilitation_aim_stop, rehabilitation_aim_resume, rehabilitation_aim_pause);
		MPAI_MessageStore_register(message_store_test_case_aiw, MPAI_AIM_Get_Subscriber(aim_rehabilitation), MOTION_DATA_CHANNEL);
		MPAI_MessageStore_register(message_store_test_case_aiw, MPAI_AIM_Get_Subscriber(aim_rehabilitation), MIC_PEAK_DATA_CHANNEL);
		mpai_error_t err_rehabilitation = MPAI_AIM_Start(aim_rehabilitation);	

		if (err_rehabilitation.code != MPAI_AIF_OK)
		{
			LOG_ERR("Error starting AIM %s: %s", log_strdup(MPAI_AIM_Get_Component(aim_rehabilitation)->name), log_strdup(MPAI_ERR_STR(err_rehabilitation.code)));
			return;
		} 
	#endif
}