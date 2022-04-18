/*
 * Copyright (c) 2022 University of Turin, Daniele Bortoluzzi <danieleb88@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef MPAI_AIW_CAE_REV_H
#define MPAI_AIW_CAE_REV_H

#include <core_aim.h>
#include <sensors_aim.h>
#include <temp_limit_aim.h>
#include <data_mic_aim.h>
#include <motion_aim.h>
#include <rehabilitation_aim.h>
#include <message_store.h>
#include <parson.h>

static int AIW_CAE_REV = 1;
#define MPAI_LIBS_CAE_REV_AIW_NAME "CAE-REV"
#define MPAI_LIBS_CAE_REV_AIM_DATA_MIC_NAME "VolumePeaksAnalysis"
#define MPAI_LIBS_CAE_REV_AIM_SENSORS_NAME "ControlUnitSensorsReading"
#define MPAI_LIBS_CAE_REV_AIM_TEMP_LIMIT_NAME "AIM_TEMP_LIMIT"
#define MPAI_LIBS_CAE_REV_AIM_MOTION_NAME "MotionRecognitionAnalysis"
#define MPAI_LIBS_CAE_REV_AIM_REHABILITATION_NAME "MovementsWithAudioValidation"
#define MPAI_LIBS_CAE_REV_AIM_COUNT 5

typedef struct _aim_initialization_cb_t{
    char* _aim_name;
    MPAI_Component_AIM_t* _aim; // at the moment it's not usefull
    module_t* _init_cb;
} aim_initialization_cb_t;

/* AIW global message store */
extern MPAI_AIM_MessageStore_t* message_store_test_case_aiw;
/* AIM message stores */
extern MPAI_AIM_MessageStore_t* message_store_data_mic_aim;
extern MPAI_AIM_MessageStore_t* message_store_sensors_aim;
extern MPAI_AIM_MessageStore_t* message_store_temp_limit_aim;
extern MPAI_AIM_MessageStore_t* message_store_motion_aim;
extern MPAI_AIM_MessageStore_t* message_store_rehabilitation_aim;

/* AIW global channels used by message store */
extern subscriber_channel_t SENSORS_DATA_CHANNEL;
extern subscriber_channel_t MIC_BUFFER_DATA_CHANNEL;
extern subscriber_channel_t MIC_PEAK_DATA_CHANNEL;

/* AIMs to be configured */
extern MPAI_Component_AIM_t* aim_produce_sensors;
extern MPAI_Component_AIM_t* aim_temp_limit;
extern MPAI_Component_AIM_t* aim_data_mic;
extern MPAI_Component_AIM_t* aim_data_motion;
extern MPAI_Component_AIM_t* aim_rehabilitation;

/**
 * @brief Initialize AIW Test Case (CAE-REV)
 * 
 */
int INIT_Test_Use_Case_AIW();

/**
 * @brief Start AIW Test Case (CAE-REV)
 * 
 */
void START_Test_Use_Case_AIW();

/**
 * @brief Stop AIW Test Case (CAE-REV)
 * 
 */
void STOP_Test_Use_Case_AIW();

/**
 * @brief Resume AIW Test Case (CAE-REV)
 */
void RESUME_Test_Use_Case_AIW();

/**
 * @brief Pause AIW Test Case (CAE-REV)
 * 
 */
void PAUSE_Test_Use_Case_AIW();

/**
 * @brief Destroy AI Test Case (CAE-REV)
 * 
 */
void DESTROY_Test_Use_Case_AIW();
#endif