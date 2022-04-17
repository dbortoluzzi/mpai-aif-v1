/*
 * @file
 * @brief Implementation of the MPAI AIW CAE-REV (Rehabilitation Exercises Validation)
 * 
 * Copyright (c) 2022 University of Turin, Daniele Bortoluzzi <danieleb88@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include "test_use_case_aiw.h"

/* AIW global message store */
MPAI_AIM_MessageStore_t* message_store_test_case_aiw;
/* AIW global channels used by message store */
subscriber_channel_t SENSORS_DATA_CHANNEL;
subscriber_channel_t MIC_BUFFER_DATA_CHANNEL;
subscriber_channel_t MIC_PEAK_DATA_CHANNEL;

void INIT_Test_Use_Case_AIW() 
{
    // create message store for the AIW
    message_store_test_case_aiw = MPAI_MessageStore_Creator(AIW_USE_CASE_ID, "AIW_CAE_REV", sizeof(mpai_parser_t));
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
}
