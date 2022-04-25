/*
 * @file
 * @brief Implementation of MPAI Config Store, according to the specs V1
 *
 * Copyright (c) 2022 University of Turin, Daniele Bortoluzzi <danieleb88@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "config_store.h"

LOG_MODULE_REGISTER(MPAI_CONFIG_STORE, LOG_LEVEL_INF);

/************* PRIVATE *************/

/************* PUBLIC **************/
char* MPAI_Config_Store_Get_AIF(const char* aif_name)
{
#ifdef CONFIG_MPAI_CONFIG_STORE_USES_COAP
	char* aif_full_name = append_strings(AIF_CONFIG[0], aif_name);
	char * aif_config_path[] = { aif_full_name, NULL };/*TODO: UNION DEFAULT OPTIONS*/
	return get_large_coap_msgs(aif_config_path);
#else
	return "{}";
#endif
} 

char* MPAI_Config_Store_Get_AIW(const char* aiw_name)
{
#ifdef CONFIG_MPAI_CONFIG_STORE_USES_COAP
	char* aiw_full_name = append_strings(AIW_CONFIG[0], aiw_name);
	char * aiw_config_path[] = { aiw_full_name, NULL };/*TODO: UNION DEFAULT OPTIONS*/
	return get_large_coap_msgs(aiw_config_path);
#else
	return "{}";
#endif
}

char* MPAI_Config_Store_Get_AIM(const char* aim_name)
{
#ifdef CONFIG_MPAI_CONFIG_STORE_USES_COAP
	char* aim_full_name = append_strings(AIM_CONFIG[0], aim_name);
	char * aim_config_path[] = { aim_full_name, NULL };/*TODO: UNION DEFAULT OPTIONS*/
	return get_large_coap_msgs(aim_config_path);
#else
	return "{}";
#endif
}