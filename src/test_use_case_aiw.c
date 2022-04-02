#include "test_use_case_aiw.h"

MPAI_AIM_MessageStore_t* message_store_test_case_aiw;
subscriber_channel_t SENSORS_DATA_CHANNEL;
subscriber_channel_t MIC_BUFFER_DATA_CHANNEL;
subscriber_channel_t MIC_PEAK_DATA_CHANNEL;

void INIT_Test_Use_Case_AIW() 
{
    message_store_test_case_aiw = MPAI_MessageStore_Creator(AIW_USE_CASE_ID, "AIF_USE_CASE", sizeof(mpai_parser_t));
    message_store_data_mic_aim = message_store_test_case_aiw;
    message_store_sensors_aim = message_store_test_case_aiw;
    message_store_temp_limit_aim = message_store_test_case_aiw;
    message_store_motion_aim = message_store_test_case_aiw;

    SENSORS_DATA_CHANNEL = MPAI_MessageStore_new_channel();
    MIC_BUFFER_DATA_CHANNEL = MPAI_MessageStore_new_channel();
    MIC_PEAK_DATA_CHANNEL = MPAI_MessageStore_new_channel();
}
