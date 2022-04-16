#include "config_store.h"

LOG_MODULE_REGISTER(MPAI_CONFIG_STORE, LOG_LEVEL_INF);

/************* PRIVATE *************/

/************* PUBLIC **************/

#ifdef CONFIG_MPAI_CONFIG_STORE && CONFIG_MPAI_CONFIG_STORE_USES_COAP
	char* MPAI_Config_Store_Get_AIF(char* aif_name)
	{
		char* aif_full_name = append_strings(AIF_CONFIG[0], aif_name);
		char * aif_config_path[] = { aif_full_name, NULL };/*TODO: UNION OPTIONS*/
		return get_large_coap_msgs(aif_config_path);
	} 
#endif