#ifndef TEST_USE_CASE_AIW_H
#define TEST_USE_CASE_AIW_H

#include <message_store.h>

static int AIW_TEMP_LIMIT_DETECTION = 1;

extern MPAI_AIM_MessageStore_t* message_store_test_case_aiw;
extern MPAI_AIM_MessageStore_t* message_store_data_mic_aim;
extern MPAI_AIM_MessageStore_t* message_store_sensors_aim;
extern MPAI_AIM_MessageStore_t* message_store_temp_limit_aim;

extern subscriber_channel_t SENSORS_DATA_CHANNEL;
extern subscriber_channel_t MIC_DATA_CHANNEL;

// #define message_store_test_case_aiw message_store_data_mic_aim
// #define message_store_test_case_aiw message_store_sensors_aim
// #define message_store_test_case_aiw message_store_temp_limit_aim


void INIT_Test_Use_Case_AIW();

#endif