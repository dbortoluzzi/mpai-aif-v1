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
char* MPAI_Config_Store_Get_AIF(char* aif_name)
{
#ifdef CONFIG_MPAI_CONFIG_STORE_USES_COAP
	char* aif_full_name = append_strings(AIF_CONFIG[0], aif_name);
	char * aif_config_path[] = { aif_full_name, NULL };/*TODO: UNION DEFAULT OPTIONS*/
	return get_large_coap_msgs(aif_config_path);
#endif
} 