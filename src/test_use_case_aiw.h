/*
 * Copyright (c) 2022 University of Turin, Daniele Bortoluzzi <danieleb88@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef TEST_USE_CASE_AIW_H
#define TEST_USE_CASE_AIW_H

#include <message_store.h>

static int AIW_USE_CASE_ID = 1;

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

/**
 * @brief Initialize AIW Test Case (CAE-REV)
 * 
 */
void INIT_Test_Use_Case_AIW();

#endif